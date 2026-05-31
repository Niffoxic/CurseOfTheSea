//=============================================================================
// Curse of the Sea
//=============================================================================
// Created by  Niffoxic - Harsh Dubey
// Module      WM9M6 Fundamentals of Games Research Development and Management
// Institution University of Warwick
//
// A linear story driven pirate adventure built from scratch in C++23 and
// DirectX 12 for the University of Warwick game project assessment.
//=============================================================================
#include "trishul/renderer/hardware/swapchain.h"
#include "trishul/core/exception/dx_exception.h"
#include "trishul/core/engine_assert.h"
#include "trishul/core/service_locator.h"
#include "trishul/event/dispatcher.h"
#include "trishul/event/render_event.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <algorithm>
#include <limits>
#include <cstdio>

#include "trishul/utils/logger.h"

namespace trishul::render::hardware
{
    namespace //~ local helpers
    {
        bool query_tearing_support(IDXGIFactory7* factory)
        {
            if (!factory) return false;
            BOOL allow = FALSE;
            if (FAILED(factory->CheckFeatureSupport(
                    DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow, sizeof(allow))))
                return false;
            return allow == TRUE;
        }
    }

    swapchain::~swapchain()
    {
        deinitialize();
    }

    bool swapchain::initialize()
    {
        //~ first call wires the config and subscribes for device swaps
        if (!device_)
        {
            const auto* cfg = config_as<swapchain_config>();
            ENGINE_ASSERT_MSG(cfg && cfg->dev,
                "swapchain config missing call set_config<swapchain_config> first");
            device_      = cfg->dev;
            create_info_ = cfg->info;
            subscribe_events();
        }

        //~ already up and nobody flagged us nothing to do
        if (swapchain_ && !need_rebuild_.load(std::memory_order_acquire)) return true;

        //~ rebuild tear the old one down first keep window state for restore
        if (swapchain_)
        {
            if (current_mode_ == display_mode::exclusive_fullscreen)
                swapchain_->SetFullscreenState(FALSE, nullptr);
            release_backbuffers();
            rtv_heap_ .Reset();
            swapchain_.Reset();
        }

        if (!build()) return false;

        need_rebuild_.store(false, std::memory_order_release);
        return true;
    }

    bool swapchain::build()
    {
        if (!create_info_.window_handle)
        {
            LOG_ERROR("null window_handle");
            return false;
        }

        window_handle_     = create_info_.window_handle;
        frame_count_       = std::clamp(create_info_.frame_count, 2u, config::MAX_BACKBUFFER_COUNT);
        tearing_supported_ = create_info_.allow_tearing &&
                             query_tearing_support(device_->dxgi_factory());

        //~ remember initial windowed state
        GetWindowRect(window_handle_, &windowed_rect_);
        windowed_style_ = GetWindowLongPtrW(window_handle_, GWL_STYLE);

        try
        {
            if (!create_swapchain(create_info_)) return false;

            //~ apply requested mode after swapchain exists windowed needs no
            //~ window move create_swapchain already sized to the window
            //~ apply_windowed is only for transitioning back out of fullscreen
            switch (create_info_.mode)
            {
            case display_mode::windowed:
                break;
            case display_mode::borderless:
                if (!apply_borderless(create_info_)) return false;
                break;
            case display_mode::exclusive_fullscreen:
                if (!apply_exclusive(create_info_)) return false;
                break;
            }

            current_mode_ = create_info_.mode;
            LOG_INFO("initialized {}x{} mode={}",
                         width_, height_, static_cast<int>(current_mode_));
            return true;
        }
        catch (const exception::directx& e)
        {
            LOG_ERROR("init failed: {}", e.what());
            release_backbuffers();
            rtv_heap_ .Reset();
            swapchain_.Reset();
            return false;
        }
    }

    void swapchain::deinitialize() noexcept
    {
        unsubscribe_events();

        if (swapchain_ && current_mode_ == display_mode::exclusive_fullscreen)
        {
            swapchain_->SetFullscreenState(FALSE, nullptr);
        }

        release_backbuffers();
        rtv_heap_ .Reset();
        swapchain_.Reset();

        window_handle_ = nullptr;
        width_  = 1;
        height_ = 1;
        current_mode_ = display_mode::windowed;
        is_occluded_  = false;
        device_       = nullptr;
        need_rebuild_.store(true, std::memory_order_release); //~ reusable
    }

    bool swapchain::create_swapchain(const swapchain_create_info& info)
    {
        auto* factory = device_->dxgi_factory();
        auto* queue   = device_->graphics_queue();
        auto* d3d     = device_->d3d12_device();

        width_  = info.width  > 0 ? info.width  : 1;
        height_ = info.height > 0 ? info.height : 1;

        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width       = width_;
        desc.Height      = height_;
        desc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc  = { 1, 0 };
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = frame_count_;
        desc.Scaling     = DXGI_SCALING_STRETCH;
        desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode   = DXGI_ALPHA_MODE_IGNORE;
        desc.Flags       = tearing_supported_ ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> sc1;
        DX_THROW_IF_FAILED_MSG(
            factory->CreateSwapChainForHwnd(
                queue, info.window_handle, &desc, nullptr, nullptr, &sc1),
            "CreateSwapChainForHwnd");

        //~ disable DXGIs alt+enter handling we drive mode changes ourselves
        DX_THROW_IF_FAILED_MSG(
            factory->MakeWindowAssociation(info.window_handle, DXGI_MWA_NO_ALT_ENTER),
            "failed to create MakeWindowAssociation");

        DX_THROW_IF_FAILED_MSG(
            sc1.As(&swapchain_), "QueryInterface IDXGISwapChain4");

        //~ RTV heap
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heap_desc.NumDescriptors = frame_count_;
        DX_THROW_IF_FAILED_MSG(
            d3d->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heap_)),
            "CreateDescriptorHeap (RTV)");

        rtv_descriptor_size_ = d3d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        return create_backbuffer_views();
    }

    bool swapchain::create_backbuffer_views()
    {
        auto* d3d = device_->d3d12_device();
        D3D12_CPU_DESCRIPTOR_HANDLE h = rtv_heap_->GetCPUDescriptorHandleForHeapStart();

        for (std::uint32_t i = 0; i < frame_count_; ++i)
        {
            try
            {
                DX_THROW_IF_FAILED_MSG(
                    swapchain_->GetBuffer(i, IID_PPV_ARGS(&backbuffers_[i])),
                    "swapchain->GetBuffer");
            }
            catch (const exception::directx& e)
            {
                LOG_ERROR("{}", e.what());
                return false;
            }

            wchar_t name[64];
            swprintf_s(name, L"COTS Backbuffer %u", i);
            backbuffers_[i]->SetName(name);

            D3D12_RENDER_TARGET_VIEW_DESC rtv{};
            rtv.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
            rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            d3d->CreateRenderTargetView(backbuffers_[i].Get(), &rtv, h);
            h.ptr += rtv_descriptor_size_;
        }
        return true;
    }

    void swapchain::release_backbuffers() noexcept
    {
        for (auto& b : backbuffers_) b.Reset();
    }

    bool swapchain::apply_windowed(const swapchain_create_info& info)
    {
        //~ if coming from exclusive exit first
        BOOL was_fullscreen = FALSE;
        swapchain_->GetFullscreenState(&was_fullscreen, nullptr);
        if (was_fullscreen)
        {
            swapchain_->SetFullscreenState(FALSE, nullptr);
        }

        //~ restore window style if needed
        const LONG_PTR cur_style = GetWindowLongPtrW(window_handle_, GWL_STYLE);
        if (cur_style != windowed_style_)
        {
            SetWindowLongPtrW(window_handle_, GWL_STYLE, windowed_style_);
        }

        //~ restore client area
        const int x = windowed_rect_.left;
        const int y = windowed_rect_.top;
        const int w = static_cast<int>(info.width);
        const int h = static_cast<int>(info.height);

        SetWindowPos(window_handle_, HWND_NOTOPMOST, x, y, w, h,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        return resize(info.width, info.height);
    }

    bool swapchain::apply_borderless(const swapchain_create_info& info)
    {
        BOOL was_fullscreen = FALSE;
        swapchain_->GetFullscreenState(&was_fullscreen, nullptr);
        if (was_fullscreen) swapchain_->SetFullscreenState(FALSE, nullptr);

        //~ find the output the window is currently on
        HMONITOR mon = MonitorFromWindow(window_handle_, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfoW(mon, &mi))
        {
            LOG_ERROR("GetMonitorInfo failed");
            return false;
        }

        const int x = mi.rcMonitor.left;
        const int y = mi.rcMonitor.top;
        const int w = mi.rcMonitor.right  - mi.rcMonitor.left;
        const int h = mi.rcMonitor.bottom - mi.rcMonitor.top;

        SetWindowLongPtrW(window_handle_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(window_handle_, HWND_TOPMOST, x, y, w, h,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        return resize(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h));
    }

    bool swapchain::apply_exclusive(const swapchain_create_info& info)
    {
        if (device_->outputs().empty())
        {
            LOG_WARN("render adapter has no outputs "
                         "(hybrid GPU) - exclusive unavailable, using borderless");
            return apply_borderless(info);
        }

        //~ find target output
        Microsoft::WRL::ComPtr<IDXGIOutput6> out;
        std::uint32_t target_idx = info.output_index;

        if (info.output_index == 0)
        {
            //~ find the primary output
            for (const auto& o : device_->outputs())
            {
                if (o.is_primary) { target_idx = o.index; break; }
            }
        }
        current_output_index_ = target_idx;

        if (!find_output(target_idx, out))
        {
            LOG_ERROR("output index {} not found", target_idx);
            return false;
        }

        //~ pick display format
        display_format target_format = info.exclusive_mode;
        if (target_format.width == 0 || target_format.height == 0)
        {
            //~ use desktop native mode
            DEVMODEW dm{};
            dm.dmSize = sizeof(dm);
            EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &dm);
            target_format.width                = dm.dmPelsWidth;
            target_format.height               = dm.dmPelsHeight;
            target_format.refresh_numerator    = dm.dmDisplayFrequency;
            target_format.refresh_denominator  = 1;
        }
        else if (target_idx < device_->outputs().size())
        {
            target_format = pick_closest_mode(device_->outputs()[target_idx], target_format);
        }

        current_format_ = target_format;

        //~ resize target
        DXGI_MODE_DESC mode_desc{};
        mode_desc.Width  = target_format.width;
        mode_desc.Height = target_format.height;
        mode_desc.RefreshRate.Numerator   = target_format.refresh_numerator;
        mode_desc.RefreshRate.Denominator = target_format.refresh_denominator;
        mode_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        if (const HRESULT rt_hr = swapchain_->ResizeTarget(&mode_desc); FAILED(rt_hr))
        {
            LOG_ERROR("ResizeTarget failed (hr=0x{:08X})",
                          static_cast<std::uint32_t>(rt_hr));
            return false;
        }

        //~ set fullscreen state
        const HRESULT fs_hr = swapchain_->SetFullscreenState(TRUE, out.Get());
        if (FAILED(fs_hr))
        {
            LOG_WARN("exclusive unavailable on this adapter "
                         "(hr=0x{:08X}, could be a hybrid GPU) - falling back to borderless",
                         static_cast<std::uint32_t>(fs_hr));
            return apply_borderless(info);
        }

        return resize(target_format.width, target_format.height);
    }

    bool swapchain::resize(std::uint32_t width, std::uint32_t height)
    {
        if (!swapchain_)
        {
            LOG_ERROR("resize called before create");
            return false;
        }
        if (width == 0 || height == 0)
        {
            LOG_ERROR("resize called with invalid size");
            return false;
        }

        release_backbuffers();

        DXGI_SWAP_CHAIN_DESC1 d{};
        swapchain_->GetDesc1(&d);

        const HRESULT hr = swapchain_->ResizeBuffers(
            frame_count_, width, height, d.Format, d.Flags);

        if (FAILED(hr))
        {
            LOG_ERROR("ResizeBuffers failed (hr=0x{:08X})",
                          static_cast<std::uint32_t>(hr));
            return false;
        }

        width_  = width;
        height_ = height;

        if (!create_backbuffer_views()) return false;

        events::publish_threadsafe<events::swapchain::resized>(width_, height_);

        LOG_INFO("resized to {}x{}", width_, height_);
        return true;
    }

    bool swapchain::recreate(const swapchain_create_info& info)
    {
        events::publish_threadsafe<events::swapchain::will_recreate>();

        //~ stash the new params and force a full rebuild through initialize
        create_info_ = info;
        mark_for_rebuild();

        if (!initialize()) return false;

        events::publish_threadsafe<events::swapchain::recreated>(
            width_, height_, current_mode_);
        return true;
    }

    bool swapchain::set_display_mode(display_mode mode)
    {
        if (mode == current_mode_) return true;

        swapchain_create_info info = create_info_;
        info.window_handle = window_handle_;
        info.width         = width_;
        info.height        = height_;
        info.mode          = mode;
        info.frame_count   = frame_count_;
        info.allow_tearing = tearing_supported_;
        info.output_index  = current_output_index_;

        bool ok = false;
        switch (mode)
        {
            case display_mode::windowed:             ok = apply_windowed  (info); break;
            case display_mode::borderless:           ok = apply_borderless(info); break;
            case display_mode::exclusive_fullscreen: ok = apply_exclusive (info); break;
        }

        if (ok)
        {
            current_mode_ = mode;
            create_info_  = info;
            events::publish_threadsafe<events::swapchain::mode_changed>(mode);
            LOG_INFO("mode {}", static_cast<int>(mode));
        }
        return ok;
    }

    bool swapchain::set_exclusive_mode(const std::uint32_t output_index,
                                       const display_format& format)
    {
        swapchain_create_info info = create_info_;
        info.window_handle  = window_handle_;
        info.width          = format.width;
        info.height         = format.height;
        info.mode           = display_mode::exclusive_fullscreen;
        info.frame_count    = frame_count_;
        info.output_index   = output_index;
        info.exclusive_mode = format;

        if (apply_exclusive(info))
        {
            current_mode_ = display_mode::exclusive_fullscreen;
            create_info_  = info;
            return true;
        }
        return false;
    }

    bool swapchain::set_windowed_size(std::uint32_t width, std::uint32_t height)
    {
        if (current_mode_ != display_mode::windowed)
        {
            LOG_WARN("set_windowed_size called outside windowed mode");
            return false;
        }
        SetWindowPos(window_handle_, nullptr, 0, 0,
                     static_cast<int>(width), static_cast<int>(height),
                     SWP_NOMOVE | SWP_NOZORDER);
        return resize(width, height);
    }

    present_result swapchain::present(const std::uint32_t sync_interval)
    {
        if (!swapchain_)    return present_result::failed;
        if (is_occluded_)   return present_result::occluded;

        const UINT flags = (sync_interval == 0 && tearing_supported_ &&
                            current_mode_ != display_mode::exclusive_fullscreen)
                         ? DXGI_PRESENT_ALLOW_TEARING : 0;

        const HRESULT hr = swapchain_->Present(sync_interval, flags);

        if (hr == DXGI_STATUS_OCCLUDED)
        {
            events::publish_threadsafe<events::swapchain::occluded>();
            is_occluded_ = true;
            return present_result::occluded;
        }

        //~ device removed lights up here first the render loop reads this
        // and routes to the dred dump path before exiting cleanly
        if (hr == DXGI_ERROR_DEVICE_REMOVED ||
            hr == DXGI_ERROR_DEVICE_RESET   ||
            hr == DXGI_ERROR_DEVICE_HUNG)
        {
            LOG_ERROR("Present device removed (hr=0x{:08X})",
                          static_cast<std::uint32_t>(hr));
            return present_result::device_removed;
        }

        if (FAILED(hr))
        {
            LOG_ERROR("Present failed (hr=0x{:08X})",
                          static_cast<std::uint32_t>(hr));
            return present_result::failed;
        }
        return present_result::success;
    }

    bool swapchain::check_occlusion()
    {
        if (!swapchain_) return false;
        if (!is_occluded_)  return true;

        //~ is the window restored or not?
        if (const HRESULT hr = swapchain_->Present(0, DXGI_PRESENT_TEST); hr == S_OK)
        {
            is_occluded_ = false;
            events::publish_threadsafe<events::swapchain::restored>();
            return true;
        }
        return false;
    }

    bool swapchain::find_output(
        const std::uint32_t index,
        Microsoft::WRL::ComPtr<IDXGIOutput6>& out) const
    {
        auto* factory = device_->dxgi_factory();
        if (!factory) return false;

        std::uint32_t global = 0;

        Microsoft::WRL::ComPtr<IDXGIAdapter1> a;
        for (UINT ai = 0;
             SUCCEEDED(factory->EnumAdapterByGpuPreference(
                 ai, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&a)));
             ++ai)
        {
            DXGI_ADAPTER_DESC1 ad{};
            a->GetDesc1(&ad);
            if (ad.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { a.Reset(); continue; }

            Microsoft::WRL::ComPtr<IDXGIOutput> o;
            for (UINT oi = 0;
                 SUCCEEDED(a->EnumOutputs(oi, &o));
                 ++oi, ++global)
            {
                if (global == index)
                {
                    return SUCCEEDED(o.As(&out));
                }
                o.Reset();
            }
            a.Reset();
        }

        return false;
    }

    display_format swapchain::pick_closest_mode(const output_info& out,
                                                const display_format& requested)
    {
        if (out.supported_modes.empty()) return requested;

        const display_format* best = &out.supported_modes[0];
        double best_score = std::numeric_limits<double>::infinity();

        for (const auto& m : out.supported_modes)
        {
            const double dw = static_cast<double>(m.width)  - static_cast<double>(requested.width);
            const double dh = static_cast<double>(m.height) - static_cast<double>(requested.height);
            const double dr = m.refresh_hz() - requested.refresh_hz();
            const double score = dw * dw + dh * dh + dr * dr * 100.0;

            if (score < best_score) { best_score = score; best = &m; }
        }
        return *best;
    }

    std::uint32_t swapchain::current_backbuffer_index() const
    {
        return swapchain_ ? swapchain_->GetCurrentBackBufferIndex() : 0;
    }

    ID3D12Resource2* swapchain::current_backbuffer() const
    {
        return swapchain_ ? backbuffers_[current_backbuffer_index()].Get() : nullptr;
    }

    std::size_t swapchain::current_rtv_handle() const
    {
        if (!rtv_heap_) return 0;
        const auto [ptr] = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
        return ptr + static_cast<std::size_t>(current_backbuffer_index()) * rtv_descriptor_size_;
    }

    IDXGISwapChain4* swapchain::dxgi_swapchain() const noexcept
    {
        return swapchain_.Get();
    }

    void swapchain::subscribe_events()
    {
        if (subscribed_) return;

        auto* dispatcher = service_locator::try_get<events::dispatcher>();
        if (!dispatcher) return;

        //~ device swap invalidates our backbuffers rebuild on the new device
        dispatcher->subscribe<events::device_recreated, &swapchain::event_device_recreated>(*this);
        subscribed_ = true;
    }

    void swapchain::unsubscribe_events()
    {
        if (!subscribed_) return;

        if (auto* dispatcher = service_locator::try_get<events::dispatcher>())
            dispatcher->unsubscribe<events::device_recreated, &swapchain::event_device_recreated>(*this);

        subscribed_ = false;
    }

    void swapchain::event_device_recreated()
    {
        //~ dispatcher thread just flag it the handler rebuild pass redoes us
        mark_for_rebuild();
    }
} // namespace trishul::render::hardware