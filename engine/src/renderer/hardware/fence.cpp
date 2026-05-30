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
#include "trishul/renderer/hardware/fence.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/utils/logger.h"
#include "trishul/core/engine_assert.h"
#include "trishul/event/dispatcher.h"
#include "trishul/event/render_event.h"
#include "trishul/core/exception/dx_exception.h"

#include <d3d12.h>

using namespace trishul;

render::hardware::fence::~fence()
{
    deinitialize();
}

bool render::hardware::fence::initialize()
{
    if (not need_rebuild_.load(std::memory_order_acquire)) return true; //~ already built

    //~ pull the pod the handler set
    const auto* cfg = config_as<fence_config>();
    ENGINE_ASSERT_MSG(cfg,      "fence config missing call set_config<fence_config> first!");
    ENGINE_ASSERT_MSG(cfg->dev, "fence config device pointer is null");

    auto* d3d = cfg->dev->d3d12_device();
    ENGINE_ASSERT_MSG(d3d, "Cant create fence probably device is not created! or nullptr");

    //~ on a rebuild continue the timeline so the old values stay completed otherwise
    //~ the cpu could wait forever for a value the new gpu will never reach
    const std::uint64_t create_value = first_init_
        ? cfg->initial_value
        : last_signaled_.load(std::memory_order_relaxed);

    //~ building into a temp so a failed create does not wipe a working fence
    Microsoft::WRL::ComPtr<ID3D12Fence1> rebuilt;
    try
    {
        DX_THROW_IF_FAILED_MSG(
            d3d->CreateFence(
                create_value,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&rebuilt)),
            "failed to create fence");
    }
    catch (const exception::directx& e)
    {
        LOG_ERROR("init failed: {}", e.what());
        return false;
    }

    //~ event is device independent create once keep across rebuilds
    if (not event_)
    {
        event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (not event_)
        {
            LOG_ERROR("failed to create event object {}",
                static_cast<std::uint32_t>(::GetLastError()));
            return false;
        }
    }

    fence_ = std::move(rebuilt); //~ swap in only after success

    if (first_init_)
    {
        last_signaled_.store(cfg->initial_value, std::memory_order_release);
        subscribe_events();
        first_init_ = false;
    }

    need_rebuild_.store(false, std::memory_order_release);
    return true;
}

void render::hardware::fence::deinitialize() noexcept
{
    unsubscribe_events();

    if (event_)
    {
        ::CloseHandle(event_);
        event_ = nullptr;
    }

    fence_.Reset();
    last_signaled_.store(0u,  std::memory_order_release);
    need_rebuild_ .store(true, std::memory_order_release); //~ reusable a fresh init rebuilds
    first_init_ = true;
}

std::uint64_t render::hardware::fence::signal(ID3D12CommandQueue* queue)
{
    ENGINE_ASSERT_MSG(queue, "Command queue passed for signaling actually ref nullptr");
    ENGINE_ASSERT_MSG(event_, "event handle is nullptr did you even initialize fence once?");

    if (need_rebuild_.load(std::memory_order_acquire))
    {
        LOG_ERROR("fence needs rebuild cannot signal");
        return 0u;
    }

    const std::uint64_t target = last_signaled_.load(std::memory_order_relaxed) + 1u;

    //~ tell the queue to bump the fence to target once it drains prior work
    if (const HRESULT hr = queue->Signal(fence_.Get(), target); FAILED(hr))
    {
        LOG_ERROR("Signal failed (hr=0x{:08X})", static_cast<std::uint32_t>(hr));
        return 0u; //~ never advanced the timeline dont gonna hand out a dead value
    }

    last_signaled_.store(target, std::memory_order_release);
    return target;
}

bool render::hardware::fence::wait(
    const std::uint64_t value,
    const std::uint32_t timeout_ms) const
{
    ENGINE_ASSERT_MSG(event_, "event handle is nullptr did you even initialize fence once?");

    //~ device gone old work became irrelevant now dont deadlock the caller
    if (need_rebuild_.load(std::memory_order_acquire)) return true;

    if (value == 0u) return true; //~ nothing was signaled

    if (value <= fence_->GetCompletedValue()) return true; //~ already finished

    if (value > last_signaled_.load(std::memory_order_relaxed))
    {
        //~ nobody gonna ask the gpu for this value it would block forever!
        LOG_WARN("wait for value {} beyond last signaled skipping", value);
        return false;
    }

    if (const HRESULT hr = fence_->SetEventOnCompletion(value, event_); FAILED(hr))
    {
        LOG_ERROR("SetEventOnCompletion failed {:08X}",
            static_cast<std::uint32_t>(hr));
        return false;
    }

    return ::WaitForSingleObject(event_, timeout_ms) == WAIT_OBJECT_0;
}

bool render::hardware::fence::gpu_wait(
    ID3D12CommandQueue* queue, const std::uint64_t value) const
{
    ENGINE_ASSERT_MSG(queue, "Command queue passed for gpu wait actually ref nullptr");

    if (need_rebuild_.load(std::memory_order_acquire))
    {
        LOG_ERROR("fence needs rebuild cannot gpu wait");
        return false;
    }

    if (value == 0u) return true; //~ nothing to wait on

    //~ the queue stalls on the gpu until the timeline reaches value
    if (const HRESULT hr = queue->Wait(fence_.Get(), value); FAILED(hr))
    {
        LOG_ERROR("queue Wait failed {:08X}", static_cast<std::uint32_t>(hr));
        return false;
    }
    return true;
}

void render::hardware::fence::flush(ID3D12CommandQueue* queue)
{
    if (const std::uint64_t target = signal(queue); target != 0u)
    {
        (void)wait(target);
    }
}

bool render::hardware::fence::is_completed(const std::uint64_t value) const
{
    //~ rebuild pending treat everything as done so callers dont stall
    if (need_rebuild_.load(std::memory_order_acquire)) return true;

    ENGINE_ASSERT_MSG(fence_, "fence isnt initialized");
    return value <= fence_->GetCompletedValue();
}

std::uint64_t render::hardware::fence::completed_value() const
{
    if (need_rebuild_.load(std::memory_order_acquire))
        return last_signaled_.load(std::memory_order_relaxed);

    ENGINE_ASSERT_MSG(fence_, "fence isnt initialized");
    return fence_->GetCompletedValue();
}

ID3D12Fence1 * render::hardware::fence::native() const noexcept
{
    if (need_rebuild_.load(std::memory_order_acquire))
    {
        LOG_ERROR("fence needs rebuild native handle is stale");
        return nullptr;
    }
    return fence_.Get();
}

void render::hardware::fence::subscribe_events()
{
    if (subscribed_) return;

    auto* dispatcher = service_locator::try_get<events::dispatcher>();
    if (not dispatcher) return;

    //~ sub to device recreated so we know to rebuild on a gpu swap
    dispatcher->subscribe<events::device_recreated, &fence::event_device_created>(*this);
    subscribed_ = true;
}

void render::hardware::fence::unsubscribe_events()
{
    if (not subscribed_) return;

    if (auto* dispatcher = service_locator::try_get<events::dispatcher>())
        dispatcher->unsubscribe<events::device_recreated, &fence::event_device_created>(*this);

    subscribed_ = false;
}

void render::hardware::fence::event_device_created()
{
    //~ runs on the dispatcher thread just flag it the owner rebuilds us
    mark_for_rebuild();
}
