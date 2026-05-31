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
#ifndef CURSEOFTHESEA_DESCRIPTOR_HEAP_H
#define CURSEOFTHESEA_DESCRIPTOR_HEAP_H

#include <atomic>
#include <cstdint>
#include <vector>
#include <wrl/client.h>
#include <d3d12.h>

#include "trishul/core/interface/hardware.h"

namespace trishul::render::hardware
{
    class device;
    
    //~ init pod info
    struct descriptor_heap_config
    {
        const device* dev      { nullptr };
        std::uint32_t capacity { 0u };
    };

    //~ bindless shader visible heap
    class descriptor_heap final: public interfaces
    {
    public:
        static constexpr std::uint32_t invalid_slot = ~0u;

         descriptor_heap() = default;
        ~descriptor_heap() override;

        descriptor_heap           (const descriptor_heap&) = delete;
        descriptor_heap           (descriptor_heap&&)      = delete;
        descriptor_heap& operator=(const descriptor_heap&) = delete;
        descriptor_heap& operator=(descriptor_heap&&)      = delete;

        //~ lifecycle - idempotent and rebuilds
        //~ itself after a gpu swap
        [[nodiscard]]
        bool initialize  ()          override;
        void deinitialize() noexcept override;

        [[nodiscard]] bool need_rebuild() const noexcept override
        {
            return need_rebuild_.load(std::memory_order_acquire);
        }
        [[nodiscard]] const char* name() const noexcept override { return "descriptor_heap"; }

        //~ flags a rebuild
        void mark_for_rebuild() noexcept
        {
            need_rebuild_.store(true, std::memory_order_release);
        }

        //~ grab a free slot
        [[nodiscard]] std::uint32_t acquire();

        //~ return slot to free list
        void release(std::uint32_t slot) noexcept;

        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle(std::uint32_t slot) const;
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle(std::uint32_t slot) const;

        [[nodiscard]] ID3D12DescriptorHeap* heap     () const noexcept;
        [[nodiscard]] std::uint32_t         capacity () const noexcept { return capacity_; }
        [[nodiscard]] std::uint32_t         in_flight() const noexcept { return in_flight_; }

    private:
        bool build();  //~ (re)create the heap on device

        //~ events
        void subscribe_events      ();
        void unsubscribe_events    ();
        void event_device_recreated();

    private:
        const device* device_            { nullptr };
        std::uint32_t requested_capacity_{ 0u };

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
        std::vector<std::uint32_t>                   free_list_;

        std::uint64_t cpu_start_ { 0u };
        std::uint64_t gpu_start_ { 0u };
        std::uint32_t stride_    { 0u };
        std::uint32_t capacity_  { 0u };
        std::uint32_t next_      { 0u };
        std::uint32_t in_flight_ { 0u };

        std::atomic<bool> need_rebuild_{ true };  //~ set by the device event
        bool              subscribed_  { false };
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_DESCRIPTOR_HEAP_H
