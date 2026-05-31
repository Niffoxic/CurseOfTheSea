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
#ifndef CURSEOFTHESEA_QUEUE_TIMELINE_H
#define CURSEOFTHESEA_QUEUE_TIMELINE_H

#include <atomic>
#include <cstdint>
#include <string>

#include "types.h"
#include "fence.h"
#include "trishul/core/interface/hardware.h"

struct ID3D12CommandQueue;

namespace trishul::render::hardware
{
    class device;
    
    struct queue_timeline_config
    {
        const device*     dev       { nullptr };
        command_list_type queue_kind{ command_list_type::direct };
        const char*       label     { nullptr };
    };

    //~ a single queue plus a monotonic fence timeline cross queue sync
    //  producer signal bumps the value consumer gpu_wait blocks that queues
    //  gpu side until the producer fence reaches the value cpu waits go
    class queue_timeline final: public interfaces
    {
    public:
         queue_timeline() = default;
        ~queue_timeline() override;

        queue_timeline           (const queue_timeline&) = delete;
        queue_timeline& operator=(const queue_timeline&) = delete;
        queue_timeline           (queue_timeline&&)      = delete;
        queue_timeline& operator=(queue_timeline&&)      = delete;

        //~ lifecycle
        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        [[nodiscard]] bool        need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override
        {
            return label_.empty() ? "queue_timeline" : label_.c_str();
        }

        //~ flag a rebuild
        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ bump the timeline
        std::uint64_t signal();

        //~ insert a gpu side wait on this queue until the producer timeline
        //  reaches value queued before the next exec on this queue
        bool gpu_wait(const queue_timeline& producer, std::uint64_t value);
        bool cpu_wait(std::uint64_t value, std::uint32_t timeout_ms = INFINITE) const;

        [[nodiscard]] bool          is_complete    (std::uint64_t value) const;
        [[nodiscard]] std::uint64_t completed_value()                    const;

        [[nodiscard]] std::uint64_t last_signaled_value() const noexcept
        {
            return fence_.latest_complete_value();
        }

        [[nodiscard]] ID3D12CommandQueue* queue() const noexcept
        {
            return queue_;
        }

        [[nodiscard]]
        const fence& timeline_fence() const noexcept
        {
            return fence_;
        }

    private:
        [[nodiscard]] ID3D12CommandQueue* fetch_queue() const; //~ from device by kind

        //~ events
        void subscribe_events     ();
        void unsubscribe_events   ();
        void event_device_recreated();

    private:
        const device*       device_    { nullptr };
        fence               fence_     {};
        ID3D12CommandQueue* queue_     { nullptr };
        command_list_type   queue_kind_{ command_list_type::direct };
        std::string         label_;

        std::atomic<bool>   need_rebuild_{ true };
        bool                subscribed_  { false };
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_QUEUE_TIMELINE_H