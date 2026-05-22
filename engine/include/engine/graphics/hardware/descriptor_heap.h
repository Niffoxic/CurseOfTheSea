// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_DESCRIPTOR_HEAP_H
#define CURSEOFTHESEA_DESCRIPTOR_HEAP_H

#include <cstdint>
#include <vector>
#include <wrl/client.h>
#include <d3d12.h>

namespace cots::graphics::hardware
{
    class device;

    //~ bindless shader visible heap
    class descriptor_heap final
    {
    public:
        static constexpr std::uint32_t invalid_slot = ~0u;

         descriptor_heap() = default;
        ~descriptor_heap();

        descriptor_heap           (const descriptor_heap&) = delete;
        descriptor_heap           (descriptor_heap&&)      = delete;
        descriptor_heap& operator=(const descriptor_heap&) = delete;
        descriptor_heap& operator=(descriptor_heap&&)      = delete;

        [[nodiscard]] bool initialize  (const device& dev, std::uint32_t capacity);
                      void deinitialize() noexcept;

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
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
        std::vector<std::uint32_t>                   free_list_;

        std::uint64_t cpu_start_ { 0 };
        std::uint64_t gpu_start_ { 0 };
        std::uint32_t stride_    { 0 };
        std::uint32_t capacity_  { 0 };
        std::uint32_t next_      { 0 };
        std::uint32_t in_flight_ { 0 };
    };
} // namespace cots::graphics::hardware

#endif //CURSEOFTHESEA_DESCRIPTOR_HEAP_H
