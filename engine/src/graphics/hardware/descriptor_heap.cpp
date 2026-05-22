// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/hardware/descriptor_heap.h"
#include "engine/graphics/hardware/device.h"

#include <d3d12.h>
#include <spdlog/spdlog.h>

namespace cots::graphics::hardware
{
    descriptor_heap::~descriptor_heap()
    {
        deinitialize();
    }

    bool descriptor_heap::initialize(const device& dev, const std::uint32_t capacity)
    {
        if (heap_) return true;

        auto* d3d = dev.d3d12_device();
        if (!d3d)
        {
            spdlog::error("[hardware:heap] device not initialized");
            return false;
        }
        if (capacity == 0)
        {
            spdlog::error("[hardware:heap] capacity must be positive");
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = capacity;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask       = 0;

        if (FAILED(d3d->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_))))
        {
            spdlog::error("[hardware:heap] CreateDescriptorHeap (CBV/SRV/UAV) failed");
            return false;
        }
        heap_->SetName(L"COTS Bindless Heap");

        stride_    = d3d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        cpu_start_ = heap_->GetCPUDescriptorHandleForHeapStart().ptr;
        gpu_start_ = heap_->GetGPUDescriptorHandleForHeapStart().ptr;
        capacity_  = capacity;
        next_      = 0;
        in_flight_ = 0;
        free_list_ .clear();
        free_list_ .reserve(64);

        spdlog::info("[hardware:heap] bindless heap ready ({} slots, stride {})",
                     capacity_, stride_);
        return true;
    }

    void descriptor_heap::deinitialize() noexcept
    {
        heap_.Reset();
        free_list_.clear();
        cpu_start_ = 0;
        gpu_start_ = 0;
        stride_    = 0;
        capacity_  = 0;
        next_      = 0;
        in_flight_ = 0;
    }

    std::uint32_t descriptor_heap::acquire()
    {
        if (!heap_) return invalid_slot;

        if (!free_list_.empty())
        {
            const auto slot = free_list_.back();
            free_list_.pop_back();
            ++in_flight_;
            return slot;
        }
        if (next_ >= capacity_)
        {
            spdlog::error("[hardware:heap] bindless heap exhausted at {} slots", capacity_);
            return invalid_slot;
        }
        const auto slot = next_++;
        ++in_flight_;
        return slot;
    }

    void descriptor_heap::release(const std::uint32_t slot) noexcept
    {
        if (slot == invalid_slot || slot >= capacity_) return;
        free_list_.push_back(slot);
        if (in_flight_ > 0) --in_flight_;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptor_heap::cpu_handle(const std::uint32_t slot) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE h{};
        h.ptr = static_cast<SIZE_T>(cpu_start_) +
                static_cast<SIZE_T>(slot) * static_cast<SIZE_T>(stride_);
        return h;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE descriptor_heap::gpu_handle(const std::uint32_t slot) const
    {
        D3D12_GPU_DESCRIPTOR_HANDLE h{};
        h.ptr = gpu_start_ +
                static_cast<UINT64>(slot) * static_cast<UINT64>(stride_);
        return h;
    }

    ID3D12DescriptorHeap* descriptor_heap::heap() const noexcept
    {
        return heap_.Get();
    }
} // namespace cots::graphics::hardware
