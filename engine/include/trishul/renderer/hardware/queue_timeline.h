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

#include "types.h"
#include "fence.h"
#include "trishul/core/interface/hardware.h"

struct ID3D12CommandQueue;

namespace trishul::render::hardware
{
    class device;

    struct queue_timeline_config
    {
        const device* dev{ nullptr };
    };

    //~ a single queue plus a monotonic fence timeline for sync each one of the kinds
    template<command_list_type Kind>
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
        [[nodiscard]] const char* name() const noexcept override { return label(); }

        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ bump the timeline
        std::uint64_t signal()
        {
            return queue_ ? fence_.signal(queue_) : 0u;
        }

        //~ insert a gpu side wait on this queue until the producer fence reaches
        //  value works across kinds pass the producers timeline_fence
        bool gpu_wait(const fence& producer_fence, std::uint64_t value);

        bool cpu_wait(const std::uint64_t value, const std::uint32_t timeout_ms = INFINITE) const
        {
            return fence_.wait(value, timeout_ms);
        }

        [[nodiscard]] bool is_complete(const std::uint64_t value) const
        {
            return fence_.is_completed(value);
        }
        [[nodiscard]] std::uint64_t completed_value() const
        {
            return fence_.completed_value();
        }
        [[nodiscard]] std::uint64_t last_signaled_value() const noexcept
        {
            return fence_.latest_complete_value();
        }

        [[nodiscard]] ID3D12CommandQueue* queue() const noexcept { return queue_; }
        [[nodiscard]] const fence&        timeline_fence() const noexcept { return fence_; }

    private:
        //~ compile time queue selection and name
        [[nodiscard]] static constexpr const char* label() noexcept
        {
            if constexpr      (Kind == command_list_type::direct)  return "graphics timeline";
            else if constexpr (Kind == command_list_type::compute) return "compute timeline";
            else                                                   return "copy timeline";
        }

        [[nodiscard]] ID3D12CommandQueue* fetch_queue() const;

    private:
        const device*       device_{ nullptr };
        fence               fence_ {};
        ID3D12CommandQueue* queue_ { nullptr };

        std::atomic<bool>   need_rebuild_{ true };
    };

    //~ the three concrete timelines for each kind of queues
    using graphics_timeline = queue_timeline<command_list_type::direct>;
    using compute_timeline  = queue_timeline<command_list_type::compute>;
    using copy_timeline     = queue_timeline<command_list_type::copy>;
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_QUEUE_TIMELINE_H