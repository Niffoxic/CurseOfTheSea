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
#include "trishul/renderer/hardware/device.h"
#include "trishul/core/engine_config.h"
#include "trishul/utils/logger.h"
#include "trishul/services.h"
#include "trishul/event/render_event.h"
#include "trishul/core/exception/dx_exception.h"

#include <dxgidebug.h>
#include <D3D12MemAlloc.h>

#include "trishul/utils/statics.h"

namespace //~ niffoxic reusable device debug chunk
{
#if COTS_DEBUG
    void enable_debug_layer()
    {
        Microsoft::WRL::ComPtr<ID3D12Debug6> debug;
        if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            LOG_WARN("D3D12 debug interface unavailable");
            return;
        }
        debug->EnableDebugLayer();
        LOG_INFO("D3D12 debug layer enabled");

#if COTS_GPU_VALIDATION_ENABLED
        //~ gpu based validation is slow but needed fpr catching descriptor
        // heap and resource issues
        debug->SetEnableGPUBasedValidation(TRUE);
        LOG_INFO("D3D12 GPU based validation enabled");
#endif
    }
#endif

#if COTS_DRED_ENABLED
    void enable_dred()
    {
        //~ gotta stop gpu from just leaving without any info
        Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dred;
        if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&dred))))
        {
           LOG_WARN("RED settings interface unavailable");
            return;
        }
        dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dred->SetPageFaultEnablement      (D3D12_DRED_ENABLEMENT_FORCED_ON);
        LOG_INFO("DRED breadcrumbs and page fault enabled");
    }
#endif
     void install_info_queue_filters(ID3D12InfoQueue1* iq)
    {
        (void)iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        (void)iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,      TRUE);

        D3D12_MESSAGE_SEVERITY hide_severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

        //~ dont need obvious stuff its just spam at this point
        D3D12_MESSAGE_ID hide_ids[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_HEAP_ADDRESS_RANGE_INTERSECTS_MULTIPLE_BUFFERS,
        };

        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumSeverities = _countof(hide_severities);
        filter.DenyList.pSeverityList = hide_severities;
        filter.DenyList.NumIDs        = _countof(hide_ids);
        filter.DenyList.pIDList       = hide_ids;
        (void)iq->PushStorageFilter(&filter);
    }

    //~ I aint typing this shh
    const char* breadcrumb_op_name(const D3D12_AUTO_BREADCRUMB_OP op) noexcept
    {
        switch (op)
        {
        case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:            return "SetMarker";
        case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:           return "BeginEvent";
        case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:             return "EndEvent";
        case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:        return "DrawInstanced";
        case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED: return "DrawIndexedInstanced";
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:      return "ExecuteIndirect";
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:             return "Dispatch";
        case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:     return "CopyBufferRegion";
        case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:    return "CopyTextureRegion";
        case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:         return "CopyResource";
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:   return "ResolveSubresource";
        case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:return "ClearRenderTargetView";
        case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:return "ClearDepthStencilView";
        case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:      return "ResourceBarrier";
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:        return "ExecuteBundle";
        case D3D12_AUTO_BREADCRUMB_OP_PRESENT:              return "Present";
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:     return "ResolveQueryData";
        default:                                            return "Op";
        }
    }
} // anonymous namespace

using namespace trishul::render::hardware;

device::~device()
{
    deinitialize();
}

bool device::initialize(const device_create_info &info)
{
    if (initialized_) return true;

#if defined(COTS_DEBUG) || defined(COTS_RELWITHDEBINFO)
    enable_debug_layer();
    constexpr UINT factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#else
    constexpr UINT factory_flags = 0;
#endif

#if COTS_DRED_ENABLED
    enable_dred();
#endif

    try
    {
        DX_THROW_IF_FAILED_MSG(
            CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory_)),
            "CreateDXGIFactory2");

        enumerate_adapters(DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, adapters_info_);

        if (!create_internal(info))
        {
            deinitialize();
            return false;
        }

        initialized_ = true;
        last_info_   = info;

        LOG_INFO("initialized on {} ({} MB VRAM)",
            adapter_info_.name,
            adapter_info_.dedicated_video_memory / (1024 * 1024));
        return true;
    }
    catch (const exception::directx& e)
    {
        LOG_ERROR("init failed: {}", e.what());
        deinitialize();
        return false;
    }
}

void device::deinitialize() noexcept
{
    if (!initialized_ && !device_) return;

#if COTS_DEBUG
    //~ before tearing the allocator dump anything d3d12ma
    if (allocator_)
    {
        WCHAR* json = nullptr;
        allocator_->BuildStatsString(&json, TRUE);
        if (json)
        {
            LOG_INFO("stats on shutdown:\n{}",
                         statics::wide_to_utf8(json));
            allocator_->FreeStatsString(json);
        }
    }
#endif

    destroy_internal();

    factory_.Reset();
    adapters_info_.clear();
    adapter_info_ = {};
    initialized_  = false;

#if COTS_DEBUG
    Microsoft::WRL::ComPtr<IDXGIDebug1> dxgi_debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug))))
    {
        (void)dxgi_debug->ReportLiveObjects(
            DXGI_DEBUG_ALL,
            static_cast<DXGI_DEBUG_RLO_FLAGS>(
                DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL
        ));
    }
#endif

    LOG_INFO("deinitialized");
}

bool device::recreate(const device_create_info& info) noexcept
{
    if (not factory_)
    {
        LOG_ERROR("factory is not initialized probably recreate called before initialization!");
        return false;
    }
    LOG_INFO("recreating...");
    events::publish_threadsafe<events::device_recreating>();
    destroy_internal();

    try
    {
        if (not create_internal(info))
        {
            LOG_ERROR("failed to recreate internal device");
            return false;
        }

        last_info_ = info;
        events::publish_threadsafe<events::device_recreated>(
            adapter_info_.adapter_index);
        LOG_INFO("recreated device on {}", adapter_info_.name);
        return true;
    }
    catch (const exception::directx& e)
    {
        LOG_ERROR("Failed to recreate device exception: {}", e.what());
        return false;
    }
}

bool device::recreate()
{
    return recreate(last_info_);
}

bool device::check_device_removed() const
{
    if (!device_) return false;

    const HRESULT hr = device_->GetDeviceRemovedReason();
    if (hr == S_OK) return false;

    LOG_ERROR("device removed (hr=0x{:08X})", static_cast<std::uint32_t>(hr));
    events::publish_threadsafe<events::device_lost>(hr);
    return true;
}

void device::refresh_adapters()
{
    if (!factory_) return;
    enumerate_adapters(DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, adapters_info_);
}

void device::dump_device_removed() const
{
      if (!device_)
    {
        LOG_ERROR("dump_device_removed called with no device");
        return;
    }

    const HRESULT reason = device_->GetDeviceRemovedReason();
    LOG_ERROR("DEVICE REMOVED POST hr=0x{:08X}",
                  static_cast<std::uint32_t>(reason));

#if COTS_DRED_ENABLED
    //~ dred breadcrumbs the last gpu operation THE CULPRIT (2nd last usually tho)
    Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
    if (FAILED(device_->QueryInterface(IID_PPV_ARGS(&dred))))
    {
        LOG_ERROR("DRED unavailable on this device");
        return;
    }

    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 crumbs{};
    if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput1(&crumbs)))
    {
        std::uint32_t node_count = 0u;
        for (const D3D12_AUTO_BREADCRUMB_NODE1* node = crumbs.pHeadAutoBreadcrumbNode;
             node != nullptr;
             node = node->pNext)
        {
            ++node_count;
            const std::uint32_t last = node->pLastBreadcrumbValue
                ? *node->pLastBreadcrumbValue
                : 0u;
            const std::uint32_t total = node->BreadcrumbCount;

            const auto name = node->pCommandListDebugNameA
                ? node->pCommandListDebugNameA
                : "unnamed";

            LOG_ERROR("command list '{}' executed {} of {} ops", name, last, total);

            //~ replay the most recent few ops
            const std::uint32_t start = (last >= 8u) ? (last - 8u) : 0u;
            for (std::uint32_t i = start; i < last && i < total; ++i)
            {
                LOG_ERROR("   op[{}] = {}", i,
                              breadcrumb_op_name(node->pCommandHistory[i]));
            }
        }
        if (node_count == 0u)
        {
            LOG_ERROR("no breadcrumb nodes available");
        }
    }
    else
    {
        LOG_ERROR("GetAutoBreadcrumbsOutput1 failed");
    }

    //~ page fault virtual address and the names of resources around it
    D3D12_DRED_PAGE_FAULT_OUTPUT1 page{};
    if (SUCCEEDED(dred->GetPageFaultAllocationOutput1(&page)))
    {
        LOG_ERROR("page fault VA = 0x{:016X}",
                      static_cast<std::uint64_t>(page.PageFaultVA));
        for (const D3D12_DRED_ALLOCATION_NODE1* n = page.pHeadExistingAllocationNode;
             n != nullptr;
             n = n->pNext)
        {
            const auto name = n->ObjectNameA ? n->ObjectNameA : "unnamed";
            LOG_ERROR("existing alloc '{}' type={}",
                          name, static_cast<int>(n->AllocationType));
        }
        for (const D3D12_DRED_ALLOCATION_NODE1* n = page.pHeadRecentFreedAllocationNode;
             n != nullptr;
             n = n->pNext)
        {
            const auto name = n->ObjectNameA ? n->ObjectNameA : "unnamed";
            LOG_ERROR("freed alloc   '{}' type={}",
                          name, static_cast<int>(n->AllocationType));
        }
    }
    else
    {
       LOG_ERROR("GetPageFaultAllocationOutput2 failed");
    }
#endif
}

ID3D12Device14 * device::d3d12_device() const noexcept
{
    return device_.Get();
}

IDXGIFactory7 * device::dxgi_factory() const noexcept
{
    return factory_.Get();
}

ID3D12CommandQueue * device::graphics_queue() const noexcept
{
    return graphics_queue_.Get();
}

ID3D12CommandQueue * device::compute_queue() const noexcept
{
    return copy_queue_.Get();
}

ID3D12CommandQueue * device::copy_queue() const noexcept
{
    return copy_queue_.Get();
}

D3D12MA::Allocator* device::allocator() const noexcept
{
    return allocator_;
}

const adapter_info & device::current_adapter_info() const noexcept
{
    return adapter_info_;
}

const std::vector<adapter_info> & device::adapters_info() const noexcept
{
    return adapters_info_;
}

bool device::is_initialized() const noexcept
{
    return initialized_;
}

void device::refresh_outputs()
{
    if (not adapter_) return;
    enumerate_outputs();
}

bool device::create_internal(const device_create_info &info)
{
    LOG_INFO("Created Internal called {} times", ++created_times_);

    if (not pick_adapter(info, adapter_))
    {
        LOG_ERROR("Failed to pick any suitable adapter!");
        return false;
    }

    DX_THROW_IF_FAILED_MSG(
        D3D12CreateDevice(adapter_.Get(), D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(&device_)),
        "Failed to create device which is near impossible btw until less its XP!");

    (void)device_->SetName(L"COTS Device");

    //~ requires enhanced barriers
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12{};
        const HRESULT hr = device_->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12));

        if (FAILED(hr) || !options12.EnhancedBarriersSupported)
        {
            LOG_ERROR(
                "enhanced barriers required but unsupported "
                "on this adapter (hr=0x{:08X}) update the GPU driver or buy a new one",
                static_cast<std::uint32_t>(hr));
            return false;
        }
        LOG_INFO("enhanced barriers supported");
    }

    //~ requires bindless
    {
        D3D12_FEATURE_DATA_SHADER_MODEL sm{ D3D_SHADER_MODEL_6_6 };
        const HRESULT hr_sm = device_->CheckFeatureSupport(
            D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm));

        if (FAILED(hr_sm) || sm.HighestShaderModel < D3D_SHADER_MODEL_6_6)
        {
            LOG_ERROR(
                "shader model 6.6 required but only "
                "0x{:X} reported update the GPU driver or buy a new one",
                static_cast<std::uint32_t>(sm.HighestShaderModel));
            return false;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
        const HRESULT hr_o = device_->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

        if (FAILED(hr_o) ||
            options.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
        {
            LOG_ERROR(
                "resource binding tier three required for "
                "bindless dynamic resources tier reported is {}",
                static_cast<int>(options.ResourceBindingTier));
            return false;
        }
        LOG_INFO("sm 6.6 and bindless supported");
    }

    //~ fill adapter info TODO: too many repeated codes
    DXGI_ADAPTER_DESC3 desc{};
    adapter_->GetDesc3(&desc);

    adapter_info_.name                   = statics::wide_to_utf8(desc.Description);
    adapter_info_.vendor_id              = desc.VendorId;
    adapter_info_.device_id              = desc.DeviceId;
    adapter_info_.dedicated_video_memory = desc.DedicatedVideoMemory;
    adapter_info_.shared_memory          = desc.SharedSystemMemory;
    adapter_info_.is_wrap                = (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) != 0;
    adapter_info_.adapter_index          = (info.manual) ? info.adapter_index : 0; // TODO: TOO RISKY for fallback!
    adapter_info_.adapter = adapter_;

    enumerate_outputs();

    //~ info queue
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&info_queue_))))
    {
        install_info_queue_filters(info_queue_.Get());
    }

     //~ graphics queue
    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
    queue_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;

    DX_THROW_IF_FAILED_MSG(
        device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&graphics_queue_)),
        "Failed to create graphics_queue_");

    (void)graphics_queue_->SetName(L"COTS Graphics Queue");

    //~ needed for concurrent fft, shadows calc etc...
    D3D12_COMMAND_QUEUE_DESC compute_desc{};
    compute_desc.Type     = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    compute_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    compute_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    compute_desc.NodeMask = 0;

    DX_THROW_IF_FAILED_MSG(
        device_->CreateCommandQueue(&compute_desc, IID_PPV_ARGS(&compute_queue_)),
        "failed to create compute_queue_");

    (void)compute_queue_->SetName(L"COTS Async Compute Queue");

    D3D12_COMMAND_QUEUE_DESC copy_desc{};
    copy_desc.Type     = D3D12_COMMAND_LIST_TYPE_COPY;
    copy_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    copy_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    copy_desc.NodeMask = 0;

    DX_THROW_IF_FAILED_MSG(
        device_->CreateCommandQueue(&copy_desc, IID_PPV_ARGS(&copy_queue_)),
        "failed to create copy_queue_");

    (void)copy_queue_->SetName(L"COTS Copy Queue");

   LOG_INFO("created three queues (graphics + async compute + copy)");

    //~ create memory allocator
    D3D12MA::ALLOCATOR_DESC alloc_desc{};
    alloc_desc.pDevice  = device_.Get();
    alloc_desc.pAdapter = adapter_.Get();

    if (FAILED(D3D12MA::CreateAllocator(&alloc_desc, &allocator_)))
    {
        LOG_ERROR("D3D12MA::CreateAllocator failed");
        return false;
    }
    return true;
}

void device::destroy_internal() noexcept
{
    if (allocator_)
    {
        allocator_->Release();
        allocator_ = nullptr;
    }

    graphics_queue_.Reset();
    compute_queue_ .Reset();
    copy_queue_    .Reset();
    info_queue_    .Reset();
    device_        .Reset();
    adapter_       .Reset();
}

void device::enumerate_adapters(
    const DXGI_GPU_PREFERENCE pref,
    std::vector<adapter_info> &out,
    const UINT flags
) const
{
    ENGINE_ASSERT_MSG(factory_, "Factory is not initialized");
    out.clear();

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter{};
    UINT adapter_index = 0u;
    while (SUCCEEDED(factory_->EnumAdapterByGpuPreference(
        adapter_index, pref,
        IID_PPV_ARGS(&adapter))))
    {
        DXGI_ADAPTER_DESC1 desc{};
        (void)adapter->GetDesc1(&desc);

        if (desc.Flags & flags)
        {
            adapter.Reset();
            continue;
        }

        adapter_info entry{};
        entry.name                   = statics::wide_to_utf8(desc.Description);
        entry.adapter_index          = adapter_index;
        entry.vendor_id              = desc.VendorId;
        entry.device_id              = desc.DeviceId;
        entry.dedicated_video_memory = desc.DedicatedVideoMemory;
        entry.shared_memory          = desc.SharedSystemMemory;
        entry.is_wrap                = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
        entry.adapter = std::move(adapter);
        out.push_back(std::move(entry));

        adapter_index++;
    }
}

void device::enumerate_outputs()
{
    ENGINE_ASSERT_MSG(factory_, "Factory is not initialized");
    ENGINE_ASSERT_MSG(adapter_, "Adapter is not initialized");

    const auto collect_from = [this](IDXGIAdapter1* adapter) -> std::uint32_t
    {
        std::uint32_t added{};
        Microsoft::WRL::ComPtr<IDXGIOutput> output{};
        std::uint32_t output_index{};

        while (SUCCEEDED(adapter->EnumOutputs(output_index, &output)))
        {
            output_index++;
            Microsoft::WRL::ComPtr<IDXGIOutput6> output6{};
            if (FAILED(output.As(&output6)))
            {
                output.Reset();
            }

            DXGI_OUTPUT_DESC1 desc{};
            if (FAILED(output6->GetDesc1(&desc)))
            {
                LOG_WARN("IDXGIOutput6::GetDesc1 failed");
                output.Reset();
                continue;
            }
            output_info entry{};
            entry.index          = static_cast<std::uint32_t>(outputs_info_.size());
            entry.device_name    = statics::wide_to_utf8(desc.DeviceName);
            entry.desktop_left   = desc.DesktopCoordinates.left;
            entry.desktop_top    = desc.DesktopCoordinates.top;
            entry.desktop_width  = static_cast<std::uint32_t>(
                desc.DesktopCoordinates.right - desc.DesktopCoordinates.left);
            entry.desktop_height = static_cast<std::uint32_t>(
                desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top);

            //~ supported modes
            constexpr DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
            UINT mode_count = 0;
            output6->GetDisplayModeList1(format, 0u, &mode_count, nullptr);

            if (mode_count)
            {
                std::vector<DXGI_MODE_DESC1> modes(mode_count);
                if (SUCCEEDED(output6->GetDisplayModeList1(
                        format, 0u, &mode_count, modes.data())))
                {
                    entry.supported_modes.reserve(mode_count);
                    for (const auto& mode : modes)
                    {
                        entry.supported_modes.push_back({
                            mode.Width,
                            mode.Height,
                            mode.RefreshRate.Numerator,
                            mode.RefreshRate.Denominator,
                        });
                    }
                    entry.native_mode = entry.supported_modes.back();
                }
            }

            entry.is_primary = (entry.desktop_left == 0 && entry.desktop_top == 0);

            outputs_info_.push_back(std::move(entry));
            ++added;
            output.Reset();
        }
        return added;
    };

    //~ try the render adapter first
    collect_from(adapter_.Get());

    //~ since laptops has hybrid gpu making my job harder THANK YOU!
    if (outputs_info_.empty())
    {
        LOG_WARN("Laptop detected!");
        MessageBoxA(nullptr,
            "Respectfully play my game on your PC!",
            "YO! PROFESSOR? PLAYING ON A LAPTOP!?", MB_OK);

        std::vector<adapter_info> request_adapters;
        enumerate_adapters(
            DXGI_GPU_PREFERENCE_UNSPECIFIED,
            request_adapters,
            DXGI_ADAPTER_FLAG_SOFTWARE //~ dont need that
        );

        for (auto& adapter_info : request_adapters)
        {
            if (const auto counts = collect_from(adapter_info.adapter.Get());
                counts > 0u)
            {
                DXGI_ADAPTER_DESC1 desc{};
                (void)adapter_info.adapter->GetDesc1(&desc);
                LOG_INFO("found {} output(s) on adapter '{}'",
                    counts, statics::wide_to_utf8(desc.Description));
            }
            adapter_info.release();
        }
    }

   LOG_INFO("enumerated {} output(s)", outputs_info_.size());
    for (const auto& o : outputs_info_)
    {
        LOG_INFO("  output [{}] {} {}x{} @ ({},{}){}",
                     o.index, o.device_name,
                     o.desktop_width, o.desktop_height,
                     o.desktop_left, o.desktop_top,
                     o.is_primary ? " [primary]" : "");
    }
}

bool device::pick_adapter(const device_create_info &info, Microsoft::WRL::ComPtr<IDXGIAdapter4> &out) const
{
    ENGINE_ASSERT_MSG(factory_, "Factory isnt initialized!");
    out.Reset();

    if (info.manual) //~ fallbacks to the default!
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> a;
        if (FAILED(factory_->EnumAdapterByGpuPreference(
                info.adapter_index,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(&a))))
        {
            LOG_ERROR("adapter index {} not found",
                          info.adapter_index);
        }else return SUCCEEDED(a.As(&out));
    }

    std::vector<adapter_info> adapter_infos{};
    enumerate_adapters(info.preference, adapter_infos, info.flags);
    Microsoft::WRL::ComPtr<ID3D12Device> test_device;

    // TODO: Fallback for feature level from 12_2 to all the way bottom!
    for (auto& adapter_info : adapter_infos)
    {
        auto* adapter = adapter_info.adapter.Get();
        if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_2,
            IID_PPV_ARGS(&test_device))))
        {
            return SUCCEEDED(adapter_info.adapter.As(&out));
        }
        adapter_info.release();
    }

    if (!info.allow_warp_fallback) return false;
    LOG_WARN("no hardware adapter falling back to WARP");

   Microsoft::WRL::ComPtr<IDXGIAdapter1> warp;
    if (FAILED(factory_->EnumWarpAdapter(IID_PPV_ARGS(&warp))))
        return false;

    return SUCCEEDED(warp.As(&out));
}
