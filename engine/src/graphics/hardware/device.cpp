#include "engine/graphics/hardware/device.h"
#include "engine/graphics/utils/exception.h"
#include "engine/events/graphics_event.h"
#include "engine/system/define_features.h"

#include <spdlog/spdlog.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

#include "engine/utils/helpers.h"

namespace
{
#if COTS_DEBUG
    void enable_debug_layer()
    {
        Microsoft::WRL::ComPtr<ID3D12Debug6> debug;
        if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            spdlog::warn("[hardware:device] D3D12 debug interface unavailable");
            return;
        }
        debug->EnableDebugLayer();
        spdlog::info("[hardware:device] D3D12 debug layer enabled");
    }
#endif

    void install_info_queue_filters(ID3D12InfoQueue1* iq)
    {
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,      TRUE);

        D3D12_MESSAGE_SEVERITY hide[] = { D3D12_MESSAGE_SEVERITY_INFO };
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumSeverities = _countof(hide);
        filter.DenyList.pSeverityList = hide;
        iq->PushStorageFilter(&filter);
    }
} // anonymous namespace

cots::graphics::hardware::device::device() = default;

cots::graphics::hardware::device::~device()
{
    deinitialize();
}

bool cots::graphics::hardware::device::initialize(const device_create_info &info)
{
    if (initialized_) return true;

#if COTS_DEBUG
    enable_debug_layer();
    constexpr UINT factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#else
    constexpr UINT factory_flags = 0;
#endif

    try
    {
        COTS_DX_THROW_IF_FAILED_MSG(
            CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory_)),
            "CreateDXGIFactory2");

        enumerate_adapters();

        if (!create_internal(info))
        {
            deinitialize();
            return false;
        }

        initialized_ = true;
        last_info_   = info;

        spdlog::info("[hardware:device] initialized on {} ({} MB VRAM)",
                     adapter_info_.name,
                     adapter_info_.dedicated_video_memory / (1024 * 1024));
        return true;
    }
    catch (const exception& e)
    {
        spdlog::error("[hardware:device] init failed: {}", e.what());
        deinitialize();
        return false;
    }
}

void cots::graphics::hardware::device::deinitialize() noexcept
{
    if (!initialized_ && !device_) return;

    destroy_internal();

    factory_.Reset();
    adapters_info_.clear();
    adapter_info_ = {};
    initialized_  = false;

#if COTS_DEBUG
    Microsoft::WRL::ComPtr<IDXGIDebug1> dxgi_debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug))))
    {
        dxgi_debug->ReportLiveObjects(
            DXGI_DEBUG_ALL,
            static_cast<DXGI_DEBUG_RLO_FLAGS>(
                DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL
                ));
    }
#endif

    spdlog::info("[hardware:device] deinitialized");
}

bool cots::graphics::hardware::device::recreate(const device_create_info &info)
{
    if (!factory_)
    {
        spdlog::error("[hardware:device] recreate called before initialize");
        return false;
    }

    spdlog::info("[hardware:device] recreating...");

    events::publish<events::graphics::device_creation_attempted>();

    destroy_internal();

    try
    {
        if (!create_internal(info))
        {
            spdlog::error("[hardware:device] recreate failed");
            return false;
        }

        last_info_ = info;
        events::publish<events::graphics::device_validated>(
            adapter_info_.adapter_index);

        spdlog::info("[hardware:device] recreated on {}", adapter_info_.name);
        return true;
    }
    catch (const exception& e)
    {
        spdlog::error("[hardware:device] recreate exception: {}", e.what());
        return false;
    }
}

bool cots::graphics::hardware::device::recreate()
{
    return recreate(last_info_);
}

bool cots::graphics::hardware::device::check_device_removed() const
{
    if (!device_) return false;

    const HRESULT hr = device_->GetDeviceRemovedReason();
    if (hr == S_OK) return false;

    spdlog::error("[hardware:device] device removed (hr=0x{:08X})",
                  static_cast<std::uint32_t>(hr));

    events::publish<events::graphics::device_lost>(hr);
    return true;
}

void cots::graphics::hardware::device::refresh_adapters()
{
    if (!factory_) return;
    enumerate_adapters();
}

ID3D12Device14 * cots::graphics::hardware::device::d3d12_device() const noexcept
{
    return device_.Get();
}

IDXGIFactory7 * cots::graphics::hardware::device::dxgi_factory() const noexcept
{
    return factory_.Get();
}

ID3D12CommandQueue * cots::graphics::hardware::device::graphics_queue() const noexcept
{
    return graphics_queue_.Get();
}

const cots::graphics::hardware::adapter_info&
    cots::graphics::hardware::device::current_adapter_info() const noexcept
{
    return adapter_info_;
}

const std::vector<cots::graphics::hardware::adapter_info>&
    cots::graphics::hardware::device:: adapters_info() const noexcept
{
    return adapters_info_;
}

bool cots::graphics::hardware::device::is_initialized() const noexcept
{
    return initialized_;
}

bool cots::graphics::hardware::device::create_internal(
    const device_create_info &info)
{
    if (!pick_adapter(info, adapter_))
    {
        spdlog::error("[hardware:device] adapter selection failed");
        return false;
    }

    COTS_DX_THROW_IF_FAILED_MSG(
        D3D12CreateDevice(adapter_.Get(),
                          D3D_FEATURE_LEVEL_12_0,
                          IID_PPV_ARGS(&device_)),
        "D3D12CreateDevice");

    device_->SetName(L"COTS Device");

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

    //~ info queue
    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&info_queue_))))
    {
        install_info_queue_filters(info_queue_.Get());
    }

    //~ graphics queue
    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;

    COTS_DX_THROW_IF_FAILED_MSG(
        device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&graphics_queue_)),
        "CreateCommandQueue");

    graphics_queue_->SetName(L"COTS Graphics Queue");

    return true;
}

bool cots::graphics::hardware::device::pick_adapter(
    const device_create_info &info,
    Microsoft::WRL::ComPtr<IDXGIAdapter4> &out) const
{
    out.Reset();

    if (info.preference == adapter_preference::manual)
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> a;
        if (FAILED(factory_->EnumAdapterByGpuPreference(
                info.adapter_index,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(&a))))
        {
            spdlog::error("[hardware:device] adapter index {} not found",
                          info.adapter_index);
            return false;
        }
        return SUCCEEDED(a.As(&out));
    }

    const DXGI_GPU_PREFERENCE pref =
        (info.preference == adapter_preference::low_power)
            ? DXGI_GPU_PREFERENCE_MINIMUM_POWER
            : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;

    Microsoft::WRL::ComPtr<IDXGIAdapter1> candidate;
    for (UINT i = 0;
         SUCCEEDED(factory_->EnumAdapterByGpuPreference(
             i, pref, IID_PPV_ARGS(&candidate)));
         ++i)
    {
        DXGI_ADAPTER_DESC1 desc{};
        candidate->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

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

    spdlog::warn("[hardware:device] no hardware adapter, falling back to WARP");
    Microsoft::WRL::ComPtr<IDXGIAdapter1> warp;
    if (FAILED(factory_->EnumWarpAdapter(IID_PPV_ARGS(&warp)))) return false;
    return SUCCEEDED(warp.As(&out));
}

void cots::graphics::hardware::device::destroy_internal() noexcept
{
    graphics_queue_.Reset();
    info_queue_    .Reset();
    device_        .Reset();
    adapter_       .Reset();
}

void cots::graphics::hardware::device::enumerate_adapters()
{
    adapters_info_.clear();

    Microsoft::WRL::ComPtr<IDXGIAdapter1> a;
    for (UINT i = 0;
         SUCCEEDED(factory_->EnumAdapterByGpuPreference(
             i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
             IID_PPV_ARGS(&a)));
         ++i)
    {
        DXGI_ADAPTER_DESC1 desc{};
        a->GetDesc1(&desc);

        adapter_info entry{};
        entry.name                   = helpers::wide_to_utf8(desc.Description);
        entry.adapter_index          = i;
        entry.vendor_id              = desc.VendorId;
        entry.device_id              = desc.DeviceId;
        entry.dedicated_video_memory = desc.DedicatedVideoMemory;
        entry.shared_memory          = desc.SharedSystemMemory;
        entry.is_wrap                = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;

        adapters_info_.push_back(std::move(entry));
        a.Reset();
    }

    spdlog::info("[hardware:device] enumerated {} adapter(s)", adapters_info_.size());
    for (const auto& adp : adapters_info_)
    {
        spdlog::info("  [{}] {}{} - {} MB VRAM",
                     adp.adapter_index, adp.name,
                     adp.is_wrap ? " (software)" : "",
                     adp.dedicated_video_memory / (1024 * 1024));
    }
}
