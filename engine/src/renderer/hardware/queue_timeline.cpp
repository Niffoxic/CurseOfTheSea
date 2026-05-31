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
#include "trishul/renderer/hardware/queue_timeline.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/utils/logger.h"
#include "trishul/core/engine_assert.h"
#include "trishul/core/service_locator.h"
#include "trishul/event/dispatcher.h"
#include "trishul/event/render_event.h"

#include <d3d12.h>

namespace trishul::render::hardware
{
    queue_timeline::~queue_timeline()
    {
        deinitialize();
    }

    bool queue_timeline::initialize()
    {
        //~ wires the config
        if (!device_)
        {
            const auto* cfg = config_as<queue_timeline_config>();
            ENGINE_ASSERT_MSG(cfg && cfg->dev,
                "queue_timeline config missing call set_config<queue_timeline_config> first");
            device_     = cfg->dev;
            queue_kind_ = cfg->queue_kind;
            label_      = cfg->label ? cfg->label : "";

            fence_config fn{};
            fn.dev = device_;
            fence_.set_config(fence_config{ device_, 0u });
            subscribe_events();
        }

        //~ idempotent and rebuilds itself later if changes are there
        if (not fence_.initialize())
        {
            LOG_ERROR("queue_timeline '{}' fence init failed", label_);
            return false;
        }

        //~ queue still good and nobody flagged us done
        if (queue_ && !need_rebuild_.load(std::memory_order_acquire)) return true;

        //~ the device queues change on a gpu swap re fetch ours
        queue_ = fetch_queue();
        if (!queue_)
        {
            LOG_ERROR("queue_timeline '{}' could not fetch its queue", label_);
            return false;
        }

        need_rebuild_.store(false, std::memory_order_release);
        return true;
    }

    void queue_timeline::deinitialize() noexcept
    {
        unsubscribe_events();
        fence_.deinitialize(); //~ owned tear it down with us

        queue_  = nullptr;
        device_ = nullptr;
        label_.clear();
        need_rebuild_.store(true, std::memory_order_release); //~ reusable
    }

    ID3D12CommandQueue* queue_timeline::fetch_queue() const
    {
        if (!device_) return nullptr;
        switch (queue_kind_)
        {
        case command_list_type::direct:  return device_->graphics_queue();
        case command_list_type::compute: return device_->compute_queue();
        case command_list_type::copy:    return device_->copy_queue();
        }
        return nullptr;
    }

    std::uint64_t queue_timeline::signal()
    {
        if (!queue_) return 0;
        return fence_.signal(queue_);
    }

    bool queue_timeline::gpu_wait(const queue_timeline& producer,
                                  const std::uint64_t   value)
    {
        if (!queue_ || !producer.fence_.native()) return false;

        if (const HRESULT hr = queue_->Wait(producer.fence_.native(), value); FAILED(hr))
        {
            LOG_ERROR("'{}' wait on '{}' value {} failed {:08X}",
                          label_, producer.label_, value,
                          static_cast<std::uint32_t>(hr));
            return false;
        }
        return true;
    }

    bool queue_timeline::cpu_wait(
        const std::uint64_t value,
        const std::uint32_t timeout_ms) const
    {
        return fence_.wait(value, timeout_ms);
    }

    bool queue_timeline::is_complete(const std::uint64_t value) const
    {
        return fence_.is_completed(value);
    }

    std::uint64_t queue_timeline::completed_value() const
    {
        return fence_.completed_value();
    }

    void queue_timeline::subscribe_events()
    {
        if (subscribed_) return;

        auto* dispatcher = service_locator::try_get<events::dispatcher>();
        if (!dispatcher) return;

        //~ rebuilds needed when the adapter changes
        dispatcher->subscribe<events::device_recreated, &queue_timeline::event_device_recreated>(*this);
        subscribed_ = true;
    }

    void queue_timeline::unsubscribe_events()
    {
        if (!subscribed_) return;

        if (auto* dispatcher = service_locator::try_get<events::dispatcher>())
            dispatcher->unsubscribe<events::device_recreated, &queue_timeline::event_device_recreated>(*this);

        subscribed_ = false;
    }

    void queue_timeline::event_device_recreated()
    {
        mark_for_rebuild();
    }
} // namespace trishul::render::hardware