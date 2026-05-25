// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/hardware/swapchain.h"
#include "engine/graphics/utils/exception.h"
#include "engine/system/feature_locator.h"
#include "engine/events/event_dispatcher.h"
#include "engine/events/graphics_event.h"
#include "engine/graphics/hardware/types.h"
#include "engine/utils/logger.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cmath>
#include <algorithm>
#include <limits>
#include <wrl/client.h>
#include <windows.h>

using namespace cots::graphics::hardware;

namespace
{
    bool query_tearing_support(IDXGIFactory7* factory)
    {
        if (!factory)
            return false;

        BOOL allow = FALSE;
        if (FAILED(factory->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allow,
                sizeof(allow))))
        {
            return false;
        }

        return allow == TRUE;
    }
}

class swapchain::implementation
{
public:
     implementation() = default;
    ~implementation();

    implementation(const implementation&) = delete;
    implementation(implementation&&)      = delete;

    implementation& operator=(const implementation&) = delete;
    implementation& operator=(implementation&&)      = delete;

    [[nodiscard]] bool initialize(const device& dev, const swapchain_create_info& info);
                  void deinitialize() noexcept;

    [[nodiscard]] bool resize(const device& dev, std::uint32_t width, std::uint32_t height);
    [[nodiscard]] bool recreate(const device& dev, const swapchain_create_info& info);

    [[nodiscard]] bool set_display_mode(const device& dev, display_mode mode);
    [[nodiscard]] bool set_exclusive_mode(
        const device& dev,
        std::uint32_t output_index,
        const display_format& format);

    [[nodiscard]] bool set_windowed_size(
        const device& dev,
        std::uint32_t width,
        std::uint32_t height);

    [[nodiscard]] present_result present(std::uint32_t sync_interval);
    [[nodiscard]] bool check_occlusion();

    [[nodiscard]] std::uint32_t current_backbuffer_index() const;
    [[nodiscard]] ID3D12Resource2* current_backbuffer() const;
    [[nodiscard]] std::size_t current_rtv_handle() const;

    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] display_mode current_mode() const noexcept;
    [[nodiscard]] const display_format& current_format() const noexcept;
    [[nodiscard]] std::uint32_t current_output_index() const noexcept;
    [[nodiscard]] bool is_occluded() const noexcept;
    [[nodiscard]] bool tearing_supported() const noexcept;

    [[nodiscard]] IDXGISwapChain4* dxgi_swapchain() const noexcept;

private:
    [[nodiscard]] bool create_swapchain(const device& dev, const swapchain_create_info& info);
    [[nodiscard]] bool create_backbuffer_views(const device& dev);
                  void release_backbuffers() noexcept;

    [[nodiscard]] bool apply_borderless(const device& dev, const swapchain_create_info& info);
    [[nodiscard]] bool apply_exclusive(const device& dev, const swapchain_create_info& info);
    [[nodiscard]] bool apply_windowed(const device& dev, const swapchain_create_info& info);

    [[nodiscard]] bool find_output(
        const device& dev,
        std::uint32_t index,
        Microsoft::WRL::ComPtr<IDXGIOutput6>& out) const;

    [[nodiscard]] static display_format pick_closest_mode(
        const output_info& out,
        const display_format& requested);

private:
    Microsoft::WRL::ComPtr<IDXGISwapChain4>      swapchain_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap_;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource2>, 8> backbuffers_;

    HWND          window_handle_        { nullptr };
    std::uint32_t width_                { 1 };
    std::uint32_t height_               { 1 };
    std::uint32_t frame_count_          { 3 };
    std::uint32_t rtv_descriptor_size_  { 0 };
    std::uint32_t current_output_index_ { 0 };

    display_mode   current_mode_   { display_mode::windowed };
    display_format current_format_ {};

    RECT     windowed_rect_  {};
    LONG_PTR windowed_style_ { 0 };

    bool tearing_supported_ { false };
    bool is_occluded_       { false };
};

#pragma region SWAP_CHAIN_MAIN

swapchain::swapchain()
    : impl_(std::make_unique<implementation>())
{
}

swapchain::~swapchain()
{
    impl_->deinitialize();
}

bool swapchain::initialize(const device& dev, const swapchain_create_info& info)
{
    return impl_->initialize(dev, info);
}

void swapchain::deinitialize() noexcept
{
    impl_->deinitialize();
}

bool swapchain::resize(
    const device& dev,
    const std::uint32_t width,
    const std::uint32_t height)
{
    return impl_->resize(dev, width, height);
}

bool swapchain::recreate(const device& dev, const swapchain_create_info& info)
{
    return impl_->recreate(dev, info);
}

bool swapchain::set_display_mode(const device& dev, const display_mode mode)
{
    return impl_->set_display_mode(dev, mode);
}

bool swapchain::set_exclusive_mode(
    const device& dev,
    const std::uint32_t output_index,
    const display_format& format)
{
    return impl_->set_exclusive_mode(dev, output_index, format);
}

bool swapchain::set_windowed_size(
    const device& dev,
    const std::uint32_t width,
    const std::uint32_t height)
{
    return impl_->set_windowed_size(dev, width, height);
}

present_result swapchain::present(const std::uint32_t sync_interval)
{
    return impl_->present(sync_interval);
}

bool swapchain::check_occlusion()
{
    return impl_->check_occlusion();
}

std::uint32_t swapchain::current_backbuffer_index() const
{
    return impl_->current_backbuffer_index();
}

ID3D12Resource2* swapchain::current_backbuffer() const
{
    return impl_->current_backbuffer();
}

std::size_t swapchain::current_rtv_handle() const
{
    return impl_->current_rtv_handle();
}

std::uint32_t swapchain::width() const noexcept
{
    return impl_->width();
}

std::uint32_t swapchain::height() const noexcept
{
    return impl_->height();
}

display_mode swapchain::current_mode() const noexcept
{
    return impl_->current_mode();
}

const display_format& swapchain::current_format() const noexcept
{
    return impl_->current_format();
}

std::uint32_t swapchain::current_output_index() const noexcept
{
    return impl_->current_output_index();
}

bool swapchain::is_occluded() const noexcept
{
    return impl_->is_occluded();
}

bool swapchain::tearing_supported() const noexcept
{
    return impl_->tearing_supported();
}

IDXGISwapChain4* swapchain::dxgi_swapchain() const noexcept
{
    return impl_->dxgi_swapchain();
}

#pragma endregion

#pragma region SWAP_CHAIN_IMPLEMENTATION

swapchain::implementation::~implementation()
{
    deinitialize();
}

bool swapchain::implementation::initialize(
    const device& dev,
    const swapchain_create_info& info)
{
    if (swapchain_)
        return true;

    if (!info.window_handle)
    {
        LOG_ERROR("[hardware:swapchain] null window_handle");
        return false;
    }

    window_handle_     = info.window_handle;
    frame_count_       = std::clamp(info.frame_count, 2u, 8u);
    tearing_supported_ = info.allow_tearing && query_tearing_support(dev.dxgi_factory());

    GetWindowRect(window_handle_, &windowed_rect_);
    windowed_style_ = GetWindowLongPtrW(window_handle_, GWL_STYLE);

    try
    {
        if (!create_swapchain(dev, info))
            return false;

        switch (info.mode)
        {
        case display_mode::windowed:
            if (!apply_windowed(dev, info))
                return false;
            break;

        case display_mode::borderless:
            if (!apply_borderless(dev, info))
                return false;
            break;

        case display_mode::exclusive_fullscreen:
            if (!apply_exclusive(dev, info))
                return false;
            break;
        }

        current_mode_ = info.mode;

        LOG_INFO(
            "[hardware:swapchain] initialized {}x{} mode={}",
            width_,
            height_,
            static_cast<int>(current_mode_)
        );

        return true;
    }
    catch (const exception& e)
    {
        LOG_ERROR("[hardware:swapchain] init failed: {}", e.what());
        deinitialize();
        return false;
    }
}

void swapchain::implementation::deinitialize() noexcept
{
    if (swapchain_ && current_mode_ == display_mode::exclusive_fullscreen)
    {
        swapchain_->SetFullscreenState(FALSE, nullptr);
    }

    release_backbuffers();

    rtv_heap_.Reset();
    swapchain_.Reset();

    window_handle_        = nullptr;
    width_                = 1;
    height_               = 1;
    frame_count_          = 3;
    rtv_descriptor_size_  = 0;
    current_output_index_ = 0;

    current_mode_   = display_mode::windowed;
    current_format_ = {};

    windowed_rect_  = {};
    windowed_style_ = 0;

    tearing_supported_ = false;
    is_occluded_       = false;
}

bool swapchain::implementation::create_swapchain(
    const device& dev,
    const swapchain_create_info& info)
{
    auto* factory = dev.dxgi_factory();
    auto* queue   = dev.graphics_queue();
    auto* d3d     = dev.d3d12_device();

    if (!factory || !queue || !d3d)
    {
        LOG_ERROR("[hardware:swapchain] create failed device is not ready");
        return false;
    }

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

    COTS_DX_THROW_IF_FAILED_MSG(
        factory->CreateSwapChainForHwnd(
            queue,
            info.window_handle,
            &desc,
            nullptr,
            nullptr,
            &sc1),
        "CreateSwapChainForHwnd"
    );

    COTS_DX_THROW_IF_FAILED_MSG(
        factory->MakeWindowAssociation(
            info.window_handle,
            DXGI_MWA_NO_ALT_ENTER),
        "MakeWindowAssociation"
    );

    COTS_DX_THROW_IF_FAILED_MSG(
        sc1.As(&swapchain_),
        "QueryInterface IDXGISwapChain4"
    );

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
    heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_desc.NumDescriptors = frame_count_;
    heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.NodeMask       = 0;

    COTS_DX_THROW_IF_FAILED_MSG(
        d3d->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heap_)),
        "CreateDescriptorHeap RTV"
    );

    rtv_descriptor_size_ =
        d3d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    return create_backbuffer_views(dev);
}

bool swapchain::implementation::create_backbuffer_views(const device& dev)
{
    auto* d3d = dev.d3d12_device();
    if (!d3d || !swapchain_ || !rtv_heap_)
    {
        LOG_ERROR("[hardware:swapchain] create_backbuffer_views called before create");
        return false;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        rtv_heap_->GetCPUDescriptorHandleForHeapStart();

    for (std::uint32_t i = 0; i < frame_count_; ++i)
    {
        try
        {
            COTS_DX_THROW_IF_FAILED_MSG(
                swapchain_->GetBuffer(i, IID_PPV_ARGS(&backbuffers_[i])),
                "swapchain GetBuffer"
            );
        }
        catch (const exception& e)
        {
            LOG_ERROR("[hardware:swapchain] {}", e.what());
            return false;
        }

        wchar_t name[64]{};
        swprintf_s(name, L"COTS Backbuffer %u", i);
        backbuffers_[i]->SetName(name);

        D3D12_RENDER_TARGET_VIEW_DESC rtv{};
        rtv.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        d3d->CreateRenderTargetView(backbuffers_[i].Get(), &rtv, handle);

        handle.ptr += rtv_descriptor_size_;
    }

    return true;
}

void swapchain::implementation::release_backbuffers() noexcept
{
    for (auto& backbuffer : backbuffers_)
    {
        backbuffer.Reset();
    }
}

bool swapchain::implementation::apply_windowed(
    const device& dev,
    const swapchain_create_info& info)
{
    if (!swapchain_)
        return false;

    BOOL was_fullscreen = FALSE;
    swapchain_->GetFullscreenState(&was_fullscreen, nullptr);

    if (was_fullscreen)
    {
        swapchain_->SetFullscreenState(FALSE, nullptr);
    }

    const LONG_PTR current_style = GetWindowLongPtrW(window_handle_, GWL_STYLE);
    if (current_style != windowed_style_)
    {
        SetWindowLongPtrW(window_handle_, GWL_STYLE, windowed_style_);
    }

    const int x = windowed_rect_.left;
    const int y = windowed_rect_.top;
    const int w = static_cast<int>(info.width);
    const int h = static_cast<int>(info.height);

    SetWindowPos(
        window_handle_,
        HWND_NOTOPMOST,
        x,
        y,
        w,
        h,
        SWP_FRAMECHANGED | SWP_SHOWWINDOW
    );

    return resize(dev, info.width, info.height);
}

bool swapchain::implementation::apply_borderless(
    const device& dev,
    const swapchain_create_info& info)
{
    if (!swapchain_)
        return false;

    BOOL was_fullscreen = FALSE;
    swapchain_->GetFullscreenState(&was_fullscreen, nullptr);

    if (was_fullscreen)
    {
        swapchain_->SetFullscreenState(FALSE, nullptr);
    }

    HMONITOR monitor = MonitorFromWindow(window_handle_, MONITOR_DEFAULTTONEAREST);

    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);

    if (!GetMonitorInfoW(monitor, &monitor_info))
    {
        LOG_ERROR("[hardware:swapchain] GetMonitorInfo failed");
        return false;
    }

    const int x = monitor_info.rcMonitor.left;
    const int y = monitor_info.rcMonitor.top;
    const int w = monitor_info.rcMonitor.right  - monitor_info.rcMonitor.left;
    const int h = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

    SetWindowLongPtrW(window_handle_, GWL_STYLE, WS_POPUP | WS_VISIBLE);

    SetWindowPos(
        window_handle_,
        HWND_TOPMOST,
        x,
        y,
        w,
        h,
        SWP_FRAMECHANGED | SWP_SHOWWINDOW
    );

    return resize(
        dev,
        static_cast<std::uint32_t>(w),
        static_cast<std::uint32_t>(h)
    );
}

bool swapchain::implementation::apply_exclusive(
    const device& dev,
    const swapchain_create_info& info)
{
    if (dev.outputs().empty())
    {
        LOG_WARN(
            "[hardware:swapchain] render adapter has no outputs hybrid gpu maybe "
            "exclusive unavailable using borderless"
        );

        return apply_borderless(dev, info);
    }

    Microsoft::WRL::ComPtr<IDXGIOutput6> output;
    std::uint32_t target_index = info.output_index;

    if (info.output_index == 0)
    {
        for (const auto& out : dev.outputs())
        {
            if (out.is_primary)
            {
                target_index = out.index;
                break;
            }
        }
    }

    current_output_index_ = target_index;

    if (!find_output(dev, target_index, output))
    {
        LOG_ERROR("[hardware:swapchain] output index {} not found", target_index);
        return false;
    }

    display_format target_format = info.exclusive_mode;

    if (target_format.width == 0 || target_format.height == 0)
    {
        DEVMODEW dm{};
        dm.dmSize = sizeof(dm);

        EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &dm);

        target_format.width               = dm.dmPelsWidth;
        target_format.height              = dm.dmPelsHeight;
        target_format.refresh_numerator   = dm.dmDisplayFrequency;
        target_format.refresh_denominator = 1;
    }
    else if (target_index < dev.outputs().size())
    {
        target_format = pick_closest_mode(dev.outputs()[target_index], target_format);
    }

    current_format_ = target_format;

    DXGI_MODE_DESC mode_desc{};
    mode_desc.Width                   = target_format.width;
    mode_desc.Height                  = target_format.height;
    mode_desc.RefreshRate.Numerator   = target_format.refresh_numerator;
    mode_desc.RefreshRate.Denominator = target_format.refresh_denominator;
    mode_desc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;

    const HRESULT resize_target_hr = swapchain_->ResizeTarget(&mode_desc);
    if (FAILED(resize_target_hr))
    {
        LOG_ERROR(
            "[hardware:swapchain] ResizeTarget failed hr=0x{:08X}",
            static_cast<std::uint32_t>(resize_target_hr)
        );

        return false;
    }

    const HRESULT fullscreen_hr = swapchain_->SetFullscreenState(TRUE, output.Get());
    if (FAILED(fullscreen_hr))
    {
        LOG_WARN(
            "[hardware:swapchain] exclusive unavailable hr=0x{:08X} using borderless",
            static_cast<std::uint32_t>(fullscreen_hr)
        );

        return apply_borderless(dev, info);
    }

    return resize(dev, target_format.width, target_format.height);
}

bool swapchain::implementation::resize(
    const device& dev,
    const std::uint32_t width,
    const std::uint32_t height)
{
    if (!swapchain_)
    {
        LOG_ERROR("[hardware:swapchain] resize called before create");
        return false;
    }

    if (width == 0 || height == 0)
    {
        LOG_ERROR("[hardware:swapchain] resize called with invalid size");
        return false;
    }

    release_backbuffers();

    DXGI_SWAP_CHAIN_DESC1 desc{};
    swapchain_->GetDesc1(&desc);

    const HRESULT hr = swapchain_->ResizeBuffers(
        frame_count_,
        width,
        height,
        desc.Format,
        desc.Flags
    );

    if (FAILED(hr))
    {
        LOG_ERROR(
            "[hardware:swapchain] ResizeBuffers failed hr=0x{:08X}",
            static_cast<std::uint32_t>(hr)
        );

        return false;
    }

    width_  = width;
    height_ = height;

    if (!create_backbuffer_views(dev))
        return false;

    events::publish_threadsafe<events::swapchain::resized>(width_, height_);

    LOG_INFO("[hardware:swapchain] resized to {}x{}", width_, height_);

    return true;
}

bool swapchain::implementation::recreate(
    const device& dev,
    const swapchain_create_info& info)
{
    if (!swapchain_)
        return initialize(dev, info);

    events::publish_threadsafe<events::swapchain::will_recreate>();

    BOOL was_fullscreen = FALSE;
    swapchain_->GetFullscreenState(&was_fullscreen, nullptr);

    if (was_fullscreen)
    {
        swapchain_->SetFullscreenState(FALSE, nullptr);
    }

    release_backbuffers();

    rtv_heap_.Reset();
    swapchain_.Reset();

    if (!initialize(dev, info))
        return false;

    events::publish_threadsafe<events::swapchain::recreated>(
        width_,
        height_,
        current_mode_
    );

    return true;
}

bool swapchain::implementation::set_display_mode(
    const device& dev,
    const display_mode mode)
{
    if (mode == current_mode_)
        return true;

    swapchain_create_info info{};
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
    case display_mode::windowed:
        ok = apply_windowed(dev, info);
        break;

    case display_mode::borderless:
        ok = apply_borderless(dev, info);
        break;

    case display_mode::exclusive_fullscreen:
        ok = apply_exclusive(dev, info);
        break;
    }

    if (ok)
    {
        current_mode_ = mode;

        events::publish_threadsafe<events::swapchain::mode_changed>(mode);

        LOG_INFO(
            "[hardware:swapchain] mode changed to {}",
            static_cast<int>(mode)
        );
    }

    return ok;
}

bool swapchain::implementation::set_exclusive_mode(
    const device& dev,
    const std::uint32_t output_index,
    const display_format& format)
{
    swapchain_create_info info{};
    info.window_handle  = window_handle_;
    info.width          = format.width;
    info.height         = format.height;
    info.mode           = display_mode::exclusive_fullscreen;
    info.frame_count    = frame_count_;
    info.output_index   = output_index;
    info.exclusive_mode = format;

    const bool ok = apply_exclusive(dev, info);

    if (ok)
    {
        current_mode_ = display_mode::exclusive_fullscreen;
        events::publish_threadsafe<events::swapchain::mode_changed>(current_mode_);
    }

    return ok;
}

bool swapchain::implementation::set_windowed_size(
    const device& dev,
    const std::uint32_t width,
    const std::uint32_t height)
{
    if (current_mode_ != display_mode::windowed)
    {
        LOG_WARN("[hardware:swapchain] set_windowed_size called outside windowed mode");
        return false;
    }

    SetWindowPos(
        window_handle_,
        nullptr,
        0,
        0,
        static_cast<int>(width),
        static_cast<int>(height),
        SWP_NOMOVE | SWP_NOZORDER
    );

    return resize(dev, width, height);
}

present_result swapchain::implementation::present(const std::uint32_t sync_interval)
{
    if (!swapchain_)
        return present_result::failed;

    if (is_occluded_)
        return present_result::occluded;

    const UINT flags =
        sync_interval == 0 &&
        tearing_supported_ &&
        current_mode_ != display_mode::exclusive_fullscreen
            ? DXGI_PRESENT_ALLOW_TEARING
            : 0;

    const HRESULT hr = swapchain_->Present(sync_interval, flags);

    if (hr == DXGI_STATUS_OCCLUDED)
    {
        events::publish_threadsafe<events::swapchain::occluded>();
        is_occluded_ = true;
        return present_result::occluded;
    }

    if (hr == DXGI_ERROR_DEVICE_REMOVED ||
        hr == DXGI_ERROR_DEVICE_RESET   ||
        hr == DXGI_ERROR_DEVICE_HUNG)
    {
        LOG_ERROR(
            "[hardware:swapchain] Present device removed hr=0x{:08X}",
            static_cast<std::uint32_t>(hr)
        );

        return present_result::device_removed;
    }

    if (FAILED(hr))
    {
        LOG_ERROR(
            "[hardware:swapchain] Present failed hr=0x{:08X}",
            static_cast<std::uint32_t>(hr)
        );

        return present_result::failed;
    }

    return present_result::success;
}

bool swapchain::implementation::check_occlusion()
{
    if (!swapchain_)
        return false;

    if (!is_occluded_)
        return true;

    const HRESULT hr = swapchain_->Present(0, DXGI_PRESENT_TEST);
    if (hr == S_OK)
    {
        is_occluded_ = false;
        events::publish_threadsafe<events::swapchain::restored>();
        return true;
    }

    return false;
}

bool swapchain::implementation::find_output(
    const device& dev,
    const std::uint32_t index,
    Microsoft::WRL::ComPtr<IDXGIOutput6>& out) const
{
    auto* factory = dev.dxgi_factory();
    if (!factory)
        return false;

    std::uint32_t global_index = 0;

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

    for (UINT adapter_index = 0;
         SUCCEEDED(factory->EnumAdapterByGpuPreference(
             adapter_index,
             DXGI_GPU_PREFERENCE_UNSPECIFIED,
             IID_PPV_ARGS(&adapter)));
         ++adapter_index)
    {
        DXGI_ADAPTER_DESC1 adapter_desc{};
        adapter->GetDesc1(&adapter_desc);

        if (adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapter.Reset();
            continue;
        }

        Microsoft::WRL::ComPtr<IDXGIOutput> output;

        for (UINT output_index = 0;
             SUCCEEDED(adapter->EnumOutputs(output_index, &output));
             ++output_index, ++global_index)
        {
            if (global_index == index)
            {
                return SUCCEEDED(output.As(&out));
            }

            output.Reset();
        }

        adapter.Reset();
    }

    return false;
}

display_format swapchain::implementation::pick_closest_mode(
    const output_info& out,
    const display_format& requested)
{
    if (out.supported_modes.empty())
        return requested;

    const display_format* best_mode = &out.supported_modes[0];
    double best_score = std::numeric_limits<double>::infinity();

    for (const auto& mode : out.supported_modes)
    {
        const double width_delta =
            static_cast<double>(mode.width) - static_cast<double>(requested.width);

        const double height_delta =
            static_cast<double>(mode.height) - static_cast<double>(requested.height);

        const double refresh_delta =
            mode.refresh_hz() - requested.refresh_hz();

        const double score =
            width_delta  * width_delta +
            height_delta * height_delta +
            refresh_delta * refresh_delta * 100.0;

        if (score < best_score)
        {
            best_score = score;
            best_mode  = &mode;
        }
    }

    return *best_mode;
}

std::uint32_t swapchain::implementation::current_backbuffer_index() const
{
    return swapchain_ ? swapchain_->GetCurrentBackBufferIndex() : 0;
}

ID3D12Resource2* swapchain::implementation::current_backbuffer() const
{
    return swapchain_
        ? backbuffers_[current_backbuffer_index()].Get()
        : nullptr;
}

std::size_t swapchain::implementation::current_rtv_handle() const
{
    if (!rtv_heap_)
        return 0;

    const D3D12_CPU_DESCRIPTOR_HANDLE handle =
        rtv_heap_->GetCPUDescriptorHandleForHeapStart();

    return handle.ptr +
        static_cast<std::size_t>(current_backbuffer_index()) *
        static_cast<std::size_t>(rtv_descriptor_size_);
}

std::uint32_t swapchain::implementation::width() const noexcept
{
    return width_;
}

std::uint32_t swapchain::implementation::height() const noexcept
{
    return height_;
}

display_mode swapchain::implementation::current_mode() const noexcept
{
    return current_mode_;
}

const display_format& swapchain::implementation::current_format() const noexcept
{
    return current_format_;
}

std::uint32_t swapchain::implementation::current_output_index() const noexcept
{
    return current_output_index_;
}

bool swapchain::implementation::is_occluded() const noexcept
{
    return is_occluded_;
}

bool swapchain::implementation::tearing_supported() const noexcept
{
    return tearing_supported_;
}

IDXGISwapChain4* swapchain::implementation::dxgi_swapchain() const noexcept
{
    return swapchain_.Get();
}

#pragma endregion
