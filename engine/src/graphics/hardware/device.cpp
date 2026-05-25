#include "engine/graphics/hardware/device.h"
#include "engine/graphics/utils/exception.h"
#include "engine/events/graphics_event.h"
#include "engine/system/define_features.h"
#include "engine/utils/logger.h"

#include <cots/cots_config.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <dxgidebug.h>

#include <cstdint>
#include <vector>

#include "engine/utils/helpers.h"

namespace
{
#if defined(COTS_DEBUG)
    void enable_debug_layer() //~ directx debugger
    {
        Microsoft::WRL::ComPtr<ID3D12Debug6> debug;
        if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            LOG_WARN("[hardware:device] D3D12 debug interface unavailable");
            return;
        }

        debug->EnableDebugLayer();
        LOG_DEBUG("[hardware:device] D3D12 debug layer enabled");

#if defined(COTS_GPU_VALIDATION_ENABLED) && COTS_GPU_VALIDATION_ENABLED
        Microsoft::WRL::ComPtr<ID3D12Debug3> debug3;
        if (SUCCEEDED(debug.As(&debug3)))
        {
            debug3->SetEnableGPUBasedValidation(TRUE);
            LOG_DEBUG("[hardware:device] D3D12 GPU based validation enabled");
        }
        else
        {
            LOG_WARN("[hardware:device] D3D12 GPU based validation unavailable");
        }
#endif
    }
#else
    void enable_debug_layer()
    {
    }
#endif

#if defined(COTS_DRED_ENABLED) && COTS_DRED_ENABLED
    void enable_dred() //~ device removed data
    {
        Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dred;
        if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&dred))))
        {
            LOG_WARN("[hardware:device] DRED settings interface unavailable");
            return;
        }

        dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dred->SetPageFaultEnablement      (D3D12_DRED_ENABLEMENT_FORCED_ON);

        LOG_DEBUG("[hardware:device] DRED breadcrumbs and page fault enabled");
    }
#else
    void enable_dred()
    {
    }
#endif

    void install_info_queue_filters(ID3D12InfoQueue1* iq)
    {
        if (!iq) return;

        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,      TRUE);

        D3D12_MESSAGE_SEVERITY hide_severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        //~ silence clear spam
        D3D12_MESSAGE_ID hide_ids[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
        };

        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumSeverities = _countof(hide_severities);
        filter.DenyList.pSeverityList = hide_severities;
        filter.DenyList.NumIDs        = _countof(hide_ids);
        filter.DenyList.pIDList       = hide_ids;

        iq->PushStorageFilter(&filter);
    }

    const char* breadcrumb_op_name(const D3D12_AUTO_BREADCRUMB_OP op) noexcept
    {
        switch (op)
        {
        case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:             return "SetMarker";
        case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:            return "BeginEvent";
        case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:              return "EndEvent";
        case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:         return "DrawInstanced";
        case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:  return "DrawIndexedInstanced";
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:       return "ExecuteIndirect";
        case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:              return "Dispatch";
        case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:      return "CopyBufferRegion";
        case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:     return "CopyTextureRegion";
        case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:          return "CopyResource";
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:    return "ResolveSubresource";
        case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW: return "ClearRenderTargetView";
        case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW: return "ClearDepthStencilView";
        case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:       return "ResourceBarrier";
        case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:         return "ExecuteBundle";
        case D3D12_AUTO_BREADCRUMB_OP_PRESENT:               return "Present";
        case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:      return "ResolveQueryData";
        default:                                             return "Op";
        }
    }
} // namespace

using namespace cots::graphics::hardware;

class device::implementation
{
public:
     implementation() = default;
    ~implementation();

    implementation(const implementation&) = delete;
    implementation(implementation&&)      = delete;

    implementation& operator=(const implementation&) = delete;
    implementation& operator=(implementation&&)      = delete;

    [[nodiscard]]
    bool initialize  (const device_create_info& info = {});
    void deinitialize() noexcept;

    [[nodiscard]] bool recreate(const device_create_info& info);
    [[nodiscard]] bool recreate();

    [[nodiscard]] bool check_device_removed() const;
                  void refresh_adapters    ();

    void dump_device_removed() const;

    [[nodiscard]] ID3D12Device14*     d3d12_device  () const noexcept;
    [[nodiscard]] IDXGIFactory7*      dxgi_factory  () const noexcept;
    [[nodiscard]] ID3D12CommandQueue* graphics_queue() const noexcept;
    [[nodiscard]] ID3D12CommandQueue* compute_queue () const noexcept;
    [[nodiscard]] ID3D12CommandQueue* copy_queue    () const noexcept;
    [[nodiscard]] D3D12MA::Allocator* allocator     () const noexcept;

    [[nodiscard]] const adapter_info&              current_adapter_info() const noexcept;
    [[nodiscard]] const std::vector<adapter_info>& adapters_info       () const noexcept;
    [[nodiscard]] bool                             is_initialized      () const noexcept;

    [[nodiscard]] const std::vector<output_info>& outputs() const noexcept
    {
        return outputs_info_;
    }

    void refresh_outputs();

private:
    [[nodiscard]] bool create_internal(const device_create_info& info);

    [[nodiscard]] bool pick_adapter(
        const device_create_info& info,
        Microsoft::WRL::ComPtr<IDXGIAdapter4>& out
    ) const;

    void destroy_internal  () noexcept;
    void enumerate_adapters();
    void enumerate_outputs ();

private:
    Microsoft::WRL::ComPtr<IDXGIFactory7>      factory_;
    Microsoft::WRL::ComPtr<IDXGIAdapter4>      adapter_;
    Microsoft::WRL::ComPtr<ID3D12Device14>     device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> graphics_queue_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> compute_queue_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> copy_queue_;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue1>   info_queue_;

    D3D12MA::Allocator* allocator_{ nullptr };

    std::vector<adapter_info> adapters_info_;
    std::vector<output_info>  outputs_info_;

    adapter_info       adapter_info_{};
    device_create_info last_info_   {};

    bool initialized_{ false };
};

#pragma region DEVICE_MAIN

device::device()
    : impl_(std::make_unique<implementation>())
{}

device::~device()
{
    if (impl_) impl_->deinitialize();
}

bool device::initialize(const device_create_info& info) const
{
    return impl_->initialize(info);
}

void device::deinitialize() const noexcept
{
    impl_->deinitialize();
}

bool device::recreate(const device_create_info& info) const
{
    return impl_->recreate(info);
}

bool device::recreate() const
{
    return impl_->recreate();
}

bool device::check_device_removed() const
{
    return impl_->check_device_removed();
}

void device::refresh_adapters() const
{
    impl_->refresh_adapters();
}

void device::dump_device_removed() const
{
    impl_->dump_device_removed();
}

ID3D12Device14* device::d3d12_device() const noexcept
{
    return impl_->d3d12_device();
}

IDXGIFactory7* device::dxgi_factory() const noexcept
{
    return impl_->dxgi_factory();
}

ID3D12CommandQueue* device::graphics_queue() const noexcept
{
    return impl_->graphics_queue();
}

ID3D12CommandQueue* device::compute_queue() const noexcept
{
    return impl_->compute_queue();
}

ID3D12CommandQueue* device::copy_queue() const noexcept
{
    return impl_->copy_queue();
}

D3D12MA::Allocator* device::allocator() const noexcept
{
    return impl_->allocator();
}

const adapter_info& device::current_adapter_info() const noexcept
{
    return impl_->current_adapter_info();
}

const std::vector<adapter_info>& device::adapters_info() const noexcept
{
    return impl_->adapters_info();
}

bool device::is_initialized() const noexcept
{
    return impl_->is_initialized();
}

const std::vector<output_info>& device::outputs() const noexcept
{
    return impl_->outputs();
}

void device::refresh_outputs() const
{
    impl_->refresh_outputs();
}

#pragma endregion

#pragma region DEVICE_IMPLEMENTATION

device::implementation::~implementation()
{
    deinitialize();
}

bool device::implementation::initialize(const device_create_info& info)
{
    if (initialized_) return true;

    enable_debug_layer();

#if defined(COTS_DEBUG)
    constexpr UINT factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#else
    constexpr UINT factory_flags = 0;
#endif

    enable_dred();

    try
    {
        COTS_DX_THROW_IF_FAILED_MSG(
            CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory_)),
            "CreateDXGIFactory2");

        enumerate_adapters();

        if (!create_internal(info))
        {
            LOG_CRITICAL("[hardware:device] create_internal failed");
            deinitialize();
            return false;
        }

        initialized_ = true;
        last_info_   = info;

        LOG_INFO("[hardware:device] initialized on {} ({} MB VRAM)",
                 adapter_info_.name,
                 adapter_info_.dedicated_video_memory / (1024 * 1024));

        return true;
    }
    catch (const exception& e)
    {
        LOG_CRITICAL("[hardware:device] init failed: {}", e.what());
        deinitialize();
        return false;
    }
}

void device::implementation::deinitialize() noexcept
{
    if (!initialized_ && !device_) return;

#if defined(COTS_DEBUG)
    if (allocator_)
    {
        WCHAR* json = nullptr;
        allocator_->BuildStatsString(&json, TRUE);

        if (json)
        {
            LOG_DEBUG("[d3d12ma] stats on shutdown:\n{}",
                      helpers::wide_to_utf8(json));

            allocator_->FreeStatsString(json);
        }
    }
#endif

    destroy_internal();

    factory_.Reset();
    adapters_info_.clear();
    outputs_info_.clear();

    adapter_info_ = {};
    last_info_    = {};
    initialized_  = false;

#if defined(COTS_DEBUG)
    Microsoft::WRL::ComPtr<IDXGIDebug1> dxgi_debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug))))
    {
        dxgi_debug->ReportLiveObjects(
            DXGI_DEBUG_ALL,
            static_cast<DXGI_DEBUG_RLO_FLAGS>(
                DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
    }
#endif

    LOG_INFO("[hardware:device] deinitialized");
}

bool device::implementation::recreate(const device_create_info& info)
{
    if (!factory_)
    {
        LOG_ERROR("[hardware:device] recreate called before initialize");
        return false;
    }

    LOG_INFO("[hardware:device] recreating...");

    events::publish_threadsafe<events::device::creation_attempted>();

    destroy_internal();

    try
    {
        if (!create_internal(info))
        {
            LOG_ERROR("[hardware:device] recreate failed");
            return false;
        }

        last_info_ = info;

        events::publish_threadsafe<events::device::validated>(
            adapter_info_.adapter_index);

        LOG_INFO("[hardware:device] recreated on {}", adapter_info_.name);
        return true;
    }
    catch (const exception& e)
    {
        LOG_ERROR("[hardware:device] recreate exception: {}", e.what());
        return false;
    }
}

bool device::implementation::recreate()
{
    return recreate(last_info_);
}

bool device::implementation::check_device_removed() const
{
    if (!device_) return false;

    const HRESULT hr = device_->GetDeviceRemovedReason();
    if (hr == S_OK) return false;

    LOG_ERROR("[hardware:device] device removed hr=0x{:08X}",
              static_cast<std::uint32_t>(hr));

    events::publish_threadsafe<events::device::lost>(hr);
    return true;
}

void device::implementation::refresh_adapters()
{
    if (!factory_) return;
    enumerate_adapters();
}

void device::implementation::dump_device_removed() const
{
    if (!device_)
    {
        LOG_ERROR("[hardware:device] dump_device_removed called with no device");
        return;
    }

    const HRESULT reason = device_->GetDeviceRemovedReason();

    LOG_ERROR("[hardware:device] DEVICE REMOVED POST MORTEM hr=0x{:08X}",
              static_cast<std::uint32_t>(reason));

#if defined(COTS_DRED_ENABLED) && COTS_DRED_ENABLED
    Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
    if (FAILED(device_->QueryInterface(IID_PPV_ARGS(&dred))))
    {
        LOG_ERROR("[hardware:device] DRED unavailable on this device");
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
                : "<unnamed>";

            LOG_ERROR("[dred] command list '{}' executed {} of {} ops",
                      name,
                      last,
                      total);

            const std::uint32_t start = (last >= 8u) ? (last - 8u) : 0u;

            for (std::uint32_t i = start; i < last && i < total; ++i)
            {
                LOG_ERROR("[dred]   op[{}] = {}",
                          i,
                          breadcrumb_op_name(node->pCommandHistory[i]));
            }
        }

        if (node_count == 0u)
        {
            LOG_ERROR("[dred] no breadcrumb nodes available");
        }
    }
    else
    {
        LOG_ERROR("[dred] GetAutoBreadcrumbsOutput1 failed");
    }

    D3D12_DRED_PAGE_FAULT_OUTPUT1 page{};
    if (SUCCEEDED(dred->GetPageFaultAllocationOutput1(&page)))
    {
        LOG_ERROR("[dred] page fault VA = 0x{:016X}",
                  static_cast<std::uint64_t>(page.PageFaultVA));

        for (const D3D12_DRED_ALLOCATION_NODE1* n = page.pHeadExistingAllocationNode;
             n != nullptr;
             n = n->pNext)
        {
            const auto name = n->ObjectNameA ? n->ObjectNameA : "<unnamed>";

            LOG_ERROR("[dred] existing alloc '{}' type={}",
                      name,
                      static_cast<int>(n->AllocationType));
        }

        for (const D3D12_DRED_ALLOCATION_NODE1* n = page.pHeadRecentFreedAllocationNode;
             n != nullptr;
             n = n->pNext)
        {
            const auto name = n->ObjectNameA ? n->ObjectNameA : "<unnamed>";

            LOG_ERROR("[dred] freed alloc '{}' type={}",
                      name,
                      static_cast<int>(n->AllocationType));
        }
    }
    else
    {
        LOG_ERROR("[dred] GetPageFaultAllocationOutput1 failed");
    }
#endif
}

ID3D12Device14* device::implementation::d3d12_device() const noexcept
{
    return device_.Get();
}

IDXGIFactory7* device::implementation::dxgi_factory() const noexcept
{
    return factory_.Get();
}

ID3D12CommandQueue* device::implementation::graphics_queue() const noexcept
{
    return graphics_queue_.Get();
}

ID3D12CommandQueue* device::implementation::compute_queue() const noexcept
{
    return compute_queue_.Get();
}

ID3D12CommandQueue* device::implementation::copy_queue() const noexcept
{
    return copy_queue_.Get();
}

D3D12MA::Allocator* device::implementation::allocator() const noexcept
{
    return allocator_;
}

const adapter_info& device::implementation::current_adapter_info() const noexcept
{
    return adapter_info_;
}

const std::vector<adapter_info>& device::implementation::adapters_info() const noexcept
{
    return adapters_info_;
}

bool device::implementation::is_initialized() const noexcept
{
    return initialized_;
}

void device::implementation::refresh_outputs()
{
    if (!adapter_) return;
    enumerate_outputs();
}

bool device::implementation::create_internal(const device_create_info& info)
{
    if (!pick_adapter(info, adapter_))
    {
        LOG_ERROR("[hardware:device] adapter selection failed");
        return false;
    }

    COTS_DX_THROW_IF_FAILED_MSG(
        D3D12CreateDevice(adapter_.Get(),
                          D3D_FEATURE_LEVEL_12_0,
                          IID_PPV_ARGS(&device_)),
        "D3D12CreateDevice");

    device_->SetName(L"COTS Device");

    //~ enhanced barriers
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12{};

        const HRESULT hr = device_->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS12,
            &options12,
            sizeof(options12));

        if (FAILED(hr) || !options12.EnhancedBarriersSupported)
        {
            LOG_ERROR("[hardware:device] enhanced barriers required but unsupported "
                      "hr=0x{:08X}",
                      static_cast<std::uint32_t>(hr));

            return false;
        }

        LOG_INFO("[hardware:device] enhanced barriers supported");
    }

    //~ shader model
    {
        D3D12_FEATURE_DATA_SHADER_MODEL sm{ D3D_SHADER_MODEL_6_6 };

        const HRESULT hr_sm = device_->CheckFeatureSupport(
            D3D12_FEATURE_SHADER_MODEL,
            &sm,
            sizeof(sm));

        if (FAILED(hr_sm) || sm.HighestShaderModel < D3D_SHADER_MODEL_6_6)
        {
            LOG_ERROR("[hardware:device] shader model 6.6 required but only "
                      "0x{:X} reported",
                      static_cast<std::uint32_t>(sm.HighestShaderModel));

            return false;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS options{};

        const HRESULT hr_options = device_->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS,
            &options,
            sizeof(options));

        if (FAILED(hr_options) ||
            options.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
        {
            LOG_ERROR("[hardware:device] resource binding tier 3 required, "
                      "reported tier is {}",
                      static_cast<int>(options.ResourceBindingTier));

            return false;
        }

        LOG_INFO("[hardware:device] shader model 6.6 and bindless supported");
    }

    DXGI_ADAPTER_DESC3 desc{};
    adapter_->GetDesc3(&desc);

    adapter_info_.name                   = helpers::wide_to_utf8(desc.Description);
    adapter_info_.vendor_id              = desc.VendorId;
    adapter_info_.device_id              = desc.DeviceId;
    adapter_info_.dedicated_video_memory = desc.DedicatedVideoMemory;
    adapter_info_.shared_memory          = desc.SharedSystemMemory;
    adapter_info_.is_wrap                = (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) != 0;
    adapter_info_.adapter_index          =
        (info.preference == adapter_preference::manual) ? info.adapter_index : 0;
    adapter_info_.adapter = adapter_;

    enumerate_outputs();

    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&info_queue_))))
    {
        install_info_queue_filters(info_queue_.Get());
    }

    //~ graphics queue
    D3D12_COMMAND_QUEUE_DESC graphics_desc{};
    graphics_desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    graphics_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    graphics_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    graphics_desc.NodeMask = 0;

    COTS_DX_THROW_IF_FAILED_MSG(
        device_->CreateCommandQueue(&graphics_desc, IID_PPV_ARGS(&graphics_queue_)),
        "CreateCommandQueue graphics");

    graphics_queue_->SetName(L"COTS Graphics Queue");

    //~ compute queue
    D3D12_COMMAND_QUEUE_DESC compute_desc{};
    compute_desc.Type     = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    compute_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    compute_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    compute_desc.NodeMask = 0;

    COTS_DX_THROW_IF_FAILED_MSG(
        device_->CreateCommandQueue(&compute_desc, IID_PPV_ARGS(&compute_queue_)),
        "CreateCommandQueue compute");

    compute_queue_->SetName(L"COTS Async Compute Queue");

    //~ copy queue
    D3D12_COMMAND_QUEUE_DESC copy_desc{};
    copy_desc.Type     = D3D12_COMMAND_LIST_TYPE_COPY;
    copy_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    copy_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    copy_desc.NodeMask = 0;

    COTS_DX_THROW_IF_FAILED_MSG(
        device_->CreateCommandQueue(&copy_desc, IID_PPV_ARGS(&copy_queue_)),
        "CreateCommandQueue copy");

    copy_queue_->SetName(L"COTS Copy Queue");

    LOG_INFO("[hardware:device] created queues graphics compute copy");

    D3D12MA::ALLOCATOR_DESC alloc_desc{};
    alloc_desc.pDevice  = device_.Get();
    alloc_desc.pAdapter = adapter_.Get();

    if (FAILED(D3D12MA::CreateAllocator(&alloc_desc, &allocator_)))
    {
        LOG_ERROR("[hardware:device] D3D12MA::CreateAllocator failed");
        return false;
    }

    return true;
}

bool device::implementation::pick_adapter(
    const device_create_info& info,
    Microsoft::WRL::ComPtr<IDXGIAdapter4>& out) const
{
    out.Reset();

    if (info.preference == adapter_preference::manual)
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        if (FAILED(factory_->EnumAdapterByGpuPreference(
                info.adapter_index,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(&adapter))))
        {
            LOG_ERROR("[hardware:device] adapter index {} not found",
                      info.adapter_index);

            return false;
        }

        return SUCCEEDED(adapter.As(&out));
    }

    const DXGI_GPU_PREFERENCE preference =
        (info.preference == adapter_preference::low_power)
            ? DXGI_GPU_PREFERENCE_MINIMUM_POWER
            : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;

    Microsoft::WRL::ComPtr<IDXGIAdapter1> candidate;

    for (UINT i = 0;
         SUCCEEDED(factory_->EnumAdapterByGpuPreference(
             i,
             preference,
             IID_PPV_ARGS(&candidate)));
         ++i)
    {
        DXGI_ADAPTER_DESC1 desc{};
        candidate->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            candidate.Reset();
            continue;
        }

        Microsoft::WRL::ComPtr<ID3D12Device> probe;
        if (SUCCEEDED(D3D12CreateDevice(candidate.Get(),
                                        D3D_FEATURE_LEVEL_12_0,
                                        IID_PPV_ARGS(&probe))))
        {
            return SUCCEEDED(candidate.As(&out));
        }

        candidate.Reset();
    }

    if (!info.allow_warp_fallback) return false;

    LOG_WARN("[hardware:device] no hardware adapter, falling back to WARP");

    Microsoft::WRL::ComPtr<IDXGIAdapter1> warp;
    if (FAILED(factory_->EnumWarpAdapter(IID_PPV_ARGS(&warp)))) return false;

    return SUCCEEDED(warp.As(&out));
}

void device::implementation::destroy_internal() noexcept
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

void device::implementation::enumerate_adapters()
{
    adapters_info_.clear();

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

    for (UINT i = 0;
         SUCCEEDED(factory_->EnumAdapterByGpuPreference(
             i,
             DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
             IID_PPV_ARGS(&adapter)));
         ++i)
    {
        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);

        adapter_info entry{};
        entry.name                   = helpers::wide_to_utf8(desc.Description);
        entry.adapter_index          = i;
        entry.vendor_id              = desc.VendorId;
        entry.device_id              = desc.DeviceId;
        entry.dedicated_video_memory = desc.DedicatedVideoMemory;
        entry.shared_memory          = desc.SharedSystemMemory;
        entry.is_wrap                = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
        entry.adapter                = std::move(adapter);

        adapters_info_.push_back(std::move(entry));
    }

    LOG_INFO("[hardware:device] enumerated {} adapter(s)",
             adapters_info_.size());

    for (const auto& adp : adapters_info_)
    {
        LOG_INFO("[hardware:device] adapter [{}] {}{} - {} MB VRAM",
                 adp.adapter_index,
                 adp.name,
                 adp.is_wrap ? " (software)" : "",
                 adp.dedicated_video_memory / (1024 * 1024));
    }
}

void device::implementation::enumerate_outputs()
{
    outputs_info_.clear();

    if (!adapter_ || !factory_) return;

    const auto collect_from = [this](IDXGIAdapter1* adapter) -> std::uint32_t
    {
        std::uint32_t added = 0;
        Microsoft::WRL::ComPtr<IDXGIOutput> output;

        for (UINT i = 0;
             SUCCEEDED(adapter->EnumOutputs(i, &output));
             ++i)
        {
            Microsoft::WRL::ComPtr<IDXGIOutput6> output6;

            if (FAILED(output.As(&output6)))
            {
                output.Reset();
                continue;
            }

            DXGI_OUTPUT_DESC1 desc{};

            if (FAILED(output6->GetDesc1(&desc)))
            {
                LOG_WARN("[hardware:device] IDXGIOutput6::GetDesc1 failed");
                output.Reset();
                continue;
            }

            output_info entry{};
            entry.index          = static_cast<std::uint32_t>(outputs_info_.size());
            entry.device_name    = helpers::wide_to_utf8(desc.DeviceName);
            entry.desktop_left   = desc.DesktopCoordinates.left;
            entry.desktop_top    = desc.DesktopCoordinates.top;
            entry.desktop_width  = static_cast<std::uint32_t>(
                desc.DesktopCoordinates.right - desc.DesktopCoordinates.left);
            entry.desktop_height = static_cast<std::uint32_t>(
                desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top);

            constexpr DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

            UINT mode_count = 0;
            output6->GetDisplayModeList1(format, 0u, &mode_count, nullptr);

            if (mode_count)
            {
                std::vector<DXGI_MODE_DESC1> modes(mode_count);

                if (SUCCEEDED(output6->GetDisplayModeList1(
                        format,
                        0u,
                        &mode_count,
                        modes.data())))
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

    collect_from(adapter_.Get());

    if (outputs_info_.empty())
    {
        LOG_WARN("[hardware:device] render adapter has no outputs, scanning all adapters");

        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        for (UINT i = 0;
             SUCCEEDED(factory_->EnumAdapterByGpuPreference(
                 i,
                 DXGI_GPU_PREFERENCE_UNSPECIFIED,
                 IID_PPV_ARGS(&adapter)));
             ++i)
        {
            DXGI_ADAPTER_DESC1 desc{};
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapter.Reset();
                continue;
            }

            if (const auto count = collect_from(adapter.Get()); count > 0)
            {
                LOG_INFO("[hardware:device] found {} output(s) on adapter '{}'",
                         count,
                         helpers::wide_to_utf8(desc.Description));
            }

            adapter.Reset();
        }
    }

    LOG_INFO("[hardware:device] enumerated {} output(s)",
             outputs_info_.size());

    for (const auto& output : outputs_info_)
    {
        LOG_INFO("[hardware:device] output [{}] {} {}x{} at ({},{}){}",
                 output.index,
                 output.device_name,
                 output.desktop_width,
                 output.desktop_height,
                 output.desktop_left,
                 output.desktop_top,
                 output.is_primary ? " [primary]" : "");
    }
}

#pragma endregion
