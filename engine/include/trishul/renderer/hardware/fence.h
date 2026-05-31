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
#ifndef CURSEOFTHESEA_FENCE_H
#define CURSEOFTHESEA_FENCE_H

#include <atomic>
#include <cstdint>
#include <wrl/client.h>
#include <windows.h>

#include "trishul/core/interface/hardware.h"

struct ID3D12Fence1;
struct ID3D12CommandQueue;

namespace trishul::render::hardware
{
    class device;
    
    //~ will be assigned via the renderer at the time of bootstrap
    struct fence_config
    {
        const device* dev          { nullptr };
        std::uint64_t initial_value{ 0u };
    };

    //~ fence cpu gpu and cross queue sync
    class fence final: public interfaces
    {
    public:
         fence() = default;
        ~fence() override;

        fence(const fence&) = delete;
        fence(fence&&)      = delete;

        fence& operator=(const fence&) = delete;
        fence& operator=(fence&&)      = delete;

        //~ lifecycle initialize, idempotent
        //~ and also rebuilds itself after a gpu swap
        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        [[nodiscard]] const char* name() const noexcept override { return "fence"; }

        //~ true when the device changed!
        bool need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }

        //~ flag a rebuild
        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ returns the value reached once the gpu finishes prior work
        //~ zero means rejected rebuild pending or queue error
        std::uint64_t signal(ID3D12CommandQueue* queue);

        //~ block the cpu until gpu reaches value - by default its infinite
        bool wait(std::uint64_t value, std::uint32_t timeout_ms = INFINITE) const;

        //~ make a queue wait on the gpu timeline cross queue sync
        bool gpu_wait(ID3D12CommandQueue* queue, std::uint64_t value) const;

        //~ signal then block until idle handy for resize and teardown
        void flush(ID3D12CommandQueue* queue);

        [[nodiscard]] //~ non blocking check
        bool is_completed(std::uint64_t value) const;

        [[nodiscard]] //~ highest value we asked the gpu to reach not yet done
        std::uint64_t latest_complete_value() const
        {
            return last_signaled_.load(std::memory_order_relaxed);
        }

        [[nodiscard]] //~ highest value the gpu has actually reached
        std::uint64_t completed_value() const;

        [[nodiscard]] //~ directx 12 fence null while a rebuild is pending
        ID3D12Fence1* native() const noexcept;

    private:
        //~ events
        void subscribe_events    ();
        void unsubscribe_events  ();
        void event_device_created();

    private:
        Microsoft::WRL::ComPtr<ID3D12Fence1> fence_{ nullptr };
        HANDLE                     event_        { nullptr };
        std::atomic<std::uint64_t> last_signaled_{ 0u };
        std::atomic<bool>          need_rebuild_ { true };  //~ set by the device event
        bool                       first_init_   { true };  //~ first build vs rebuild
        bool                       subscribed_   { false }; //~ guards the dispatcher hookup
    };
} // namespace render::hardware

#endif //CURSEOFTHESEA_FENCE_H
