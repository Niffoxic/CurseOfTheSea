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
#include "trishul/renderer/hardware/command_pool.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/utils/logger.h"

namespace trishul::render::hardware
{
    command_pool::~command_pool()
    {
        deinitialize();
    }

    std::size_t command_pool::queue_index(const command_list_type type) noexcept
    {
        switch (type)
        {
        case command_list_type::compute: return 1u;
        case command_list_type::copy:    return 2u;
        case command_list_type::direct:
        default:                         return 0u;
        }
    }

    bool command_pool::initialize()
    {
        const auto* cfg = config_as<command_pool_config>();
        if (!cfg || !cfg->dev)
        {
            LOG_ERROR("command_pool missing config or device");
            return false;
        }

        //~ device may have come back on a new adapter so drop whatever the old
        //~ one handed us before will be starting pooling lists on the fresh device
        device_ = cfg->dev;
        for (auto& queue : slots_)
            for (auto& s : queue)
            {
                s.pool.clear();
                s.used = 0u;
            }

        need_rebuild_.store(false, std::memory_order_release);
        return true;
    }

    void command_pool::deinitialize() noexcept
    {
        //~ drop every list and allocator they belong to a device thats going away
        for (auto& queue : slots_)
            for (auto& s : queue)
            {
                s.pool.clear();
                s.used = 0u;
            }
        device_ = nullptr;
    }

    void command_pool::begin_frame(const std::uint32_t frame_index) noexcept
    {
        if (frame_index >= config::FRAME_COUNT) return;

        //~ just rewind the contexts stick around to be reused
        for (auto& queue : slots_)
            queue[frame_index].used = 0u;
    }

    command_context* command_pool::acquire(
        const command_list_type type,
        const std::uint32_t     frame_index)
    {
        if (!device_)
        {
            LOG_ERROR("command_pool acquire before initialize");
            return nullptr;
        }
        if (frame_index >= config::FRAME_COUNT)
        {
            LOG_ERROR("command_pool acquire bad frame index {}", frame_index);
            return nullptr;
        }

        slot& s = slots_[queue_index(type)][frame_index];

        //~ ran out of pooled lists this frame so grow by one on the device
        if (s.used >= s.pool.size())
        {
            auto ctx = std::make_unique<command_context>();
            ctx->set_config(command_context_config{ device_, type });
            if (!ctx->initialize())
            {
                LOG_ERROR("command_pool failed to create a {} context",
                          to_string(type));
                return nullptr;
            }
            s.pool.push_back(std::move(ctx));
        }

        command_context* ctx = s.pool[s.used].get();
        ++s.used;

        //~ open it for recording reusing this slots allocator from last cycle
        if (!ctx->reset()) return nullptr;
        return ctx;
    }

    command_pool_counters command_pool::counters() const noexcept
    {
        command_pool_counters c{};

        auto gather = [](const std::array<slot, config::FRAME_COUNT>& queue,
                         std::uint32_t& allocated, std::uint32_t& in_use)
        {
            for (const auto& s : queue)
            {
                allocated += static_cast<std::uint32_t>(s.pool.size());
                in_use    += s.used;
            }
        };

        gather(slots_[0], c.direct_allocated,  c.direct_in_use);
        gather(slots_[1], c.compute_allocated, c.compute_in_use);
        gather(slots_[2], c.copy_allocated,    c.copy_in_use);
        return c;
    }
} // namespace trishul::render::hardware
