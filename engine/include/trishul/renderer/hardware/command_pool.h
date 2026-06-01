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
#ifndef CURSEOFTHESEA_COMMAND_POOL_H
#define CURSEOFTHESEA_COMMAND_POOL_H

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "types.h"
#include "command_context.h"
#include "trishul/core/engine_config.h"
#include "trishul/core/interface/hardware.h"

namespace trishul::render::hardware
{
    class device;

    //~ init config only need device tho
    struct command_pool_config
    {
        const device* dev{ nullptr };
    };

    //~ how many contexts we are holding onto handy for the debug overlay so
    //~ can watch the pool grow and see if a frame is spamming lists
    struct command_pool_counters
    {
        std::uint32_t direct_allocated { 0u };
        std::uint32_t compute_allocated{ 0u };
        std::uint32_t copy_allocated   { 0u };
        std::uint32_t direct_in_use    { 0u };
        std::uint32_t compute_in_use   { 0u };
        std::uint32_t copy_in_use      { 0u };
    };

    //~ owns the per frame per queue command_context pools so the renderer can
    //~ hand them out lazily and recycle them every frame it rides the hardware
    //~ interface so a device swap tears it down and rebuilds it for free
    class command_pool final : public interfaces
    {
    public:
         command_pool() = default;
        ~command_pool() override;

        command_pool(const command_pool&) = delete;
        command_pool(command_pool&&)      = delete;

        command_pool& operator=(const command_pool&) = delete;
        command_pool& operator=(command_pool&&)      = delete;

        //~ lifecycle
        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        [[nodiscard]] bool need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override { return "command_pool"; }

        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ recycle every queue on this frame slot caller must have waited on the
        //~ frame fence first or the gpu is still reading these allocators
        void begin_frame(std::uint32_t frame_index) noexcept;

        //~ hand back a freshly reset context ready to record grows the pool if
        //~ this frame asked for more than we had returns null if creation failed
        [[nodiscard]] command_context* acquire(command_list_type type, std::uint32_t frame_index);

        [[nodiscard]] command_pool_counters counters() const noexcept;

    private:
        static constexpr std::size_t queue_count = 3u;

        struct slot
        {
            std::vector<std::unique_ptr<command_context>> pool;
            std::uint32_t used{ 0u };
        };

        //~ direct compute copy on the outer axis
        std::array<std::array<slot, config::FRAME_COUNT>, queue_count> slots_{};

        const device*     device_{ nullptr };
        std::atomic<bool> need_rebuild_{ true };

        [[nodiscard]] static std::size_t queue_index(command_list_type type) noexcept;
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_COMMAND_POOL_H
