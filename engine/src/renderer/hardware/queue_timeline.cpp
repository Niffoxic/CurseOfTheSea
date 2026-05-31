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

#include <d3d12.h>

namespace trishul::render::hardware
{
    template<command_list_type Kind>
    queue_timeline<Kind>::~queue_timeline()
    {
        deinitialize();
    }

    template<command_list_type Kind>
    bool queue_timeline<Kind>::initialize()
    {
        //~ first call wires the config configures the owned fence and subscribes
        if (!device_)
        {
            const auto* cfg = config_as<queue_timeline_config>();
            ENGINE_ASSERT_MSG(cfg && cfg->dev,
                "queue_timeline config missing call set_config<queue_timeline_config> first");
            device_ = cfg->dev;

            //~ the owned fence builds on the same device we keep driving its
            //~ initialize the handler graph rebuild handles device swaps
            fence_.set_config(fence_config{ device_, 0u });
        }

        //~ owned fence is idempotent and rebuilds itself when its flag is set
        if (not fence_.initialize())
        {
            LOG_ERROR("queue_timeline '{}' fence init failed", label());
            return false;
        }

        //~ queue still good and nobody flagged us done
        if (queue_ && !need_rebuild_.load(std::memory_order_acquire)) return true;

        //~ the device queues change on a gpu swap re fetch ours
        queue_ = fetch_queue();
        if (!queue_)
        {
            LOG_ERROR("queue_timeline '{}' could not fetch its queue", label());
            return false;
        }

        need_rebuild_.store(false, std::memory_order_release);
        return true;
    }

    template<command_list_type Kind>
    void queue_timeline<Kind>::deinitialize() noexcept
    {
        fence_.deinitialize(); //~ owned tear it down with us

        queue_  = nullptr;
        device_ = nullptr;
        need_rebuild_.store(true, std::memory_order_release); //~ reusable
    }

    template<command_list_type Kind>
    ID3D12CommandQueue* queue_timeline<Kind>::fetch_queue() const
    {
        if (!device_) return nullptr;

        if constexpr (Kind == command_list_type::direct)
            return device_->graphics_queue();
        else if constexpr (Kind == command_list_type::compute)
            return device_->compute_queue();
        else
            return device_->copy_queue();
    }

    template<command_list_type Kind>
    bool queue_timeline<Kind>::gpu_wait(const fence& producer_fence, const std::uint64_t value)
    {
        if (!queue_) return false;

        auto* native = producer_fence.native();
        if (!native) return false;

        if (const HRESULT hr = queue_->Wait(native, value); FAILED(hr))
        {
            LOG_ERROR("'{}' gpu wait on value {} failed {:08X}",
                          label(), value, static_cast<std::uint32_t>(hr));
            return false;
        }
        return true;
    }

    //~ the only three kinds that ever exist instantiate them here so render can
    //~ link against them without pulling the definitions into its translation unit
    template class queue_timeline<command_list_type::direct>;
    template class queue_timeline<command_list_type::compute>;
    template class queue_timeline<command_list_type::copy>;
} // namespace trishul::render::hardware