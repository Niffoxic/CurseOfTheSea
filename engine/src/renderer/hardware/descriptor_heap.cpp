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
#include "trishul/renderer/hardware/descriptor_heap.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/utils/logger.h"
#include "trishul/core/engine_assert.h"

#include <d3d12.h>

namespace trishul::render::hardware
{
    descriptor_heap::~descriptor_heap()
    {
        deinitialize();
    }

    bool descriptor_heap::initialize()
    {
        //~ first call wires the config and subscribes for device swaps
        if (!device_)
        {
            const auto* cfg = config_as<descriptor_heap_config>();
            ENGINE_ASSERT_MSG(cfg && cfg->dev,
                "descriptor heap config missing call set_config<descriptor_heap_config> first");
            device_             = cfg->dev;
            requested_capacity_ = cfg->capacity;
        }

        //~ already up and nobody flagged this so nothingelse to do
        if (heap_ && !need_rebuild_.load(std::memory_order_acquire)) return true;

        //~ build resets the heap and the free list a rebuild starts fresh
        if (!build()) return false;

        need_rebuild_.store(false, std::memory_order_release);
        return true;
    }

    bool descriptor_heap::build()
    {
        auto* d3d = device_->d3d12_device();
        if (!d3d)
        {
            LOG_ERROR("device not initialized");
            return false;
        }
        if (requested_capacity_ == 0)
        {
            LOG_ERROR("capacity must be positive");
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = requested_capacity_;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask       = 0;

        if (FAILED(d3d->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_))))
        {
            LOG_ERROR("CreateDescriptorHeap (CBV/SRV/UAV) failed");
            return false;
        }
        (void)heap_->SetName(L"Engine Bindless Heap");

        stride_    = d3d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        cpu_start_ = heap_->GetCPUDescriptorHandleForHeapStart().ptr;
        gpu_start_ = heap_->GetGPUDescriptorHandleForHeapStart().ptr;
        capacity_  = requested_capacity_;
        next_      = 0;
        in_flight_ = 0;
        free_list_ .clear();
        free_list_ .reserve(64);

        LOG_INFO("bindless heap ready ({} slots, stride {})", capacity_, stride_);
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
        device_    = nullptr;
        need_rebuild_.store(true, std::memory_order_release); //~ reusable
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
            LOG_ERROR("bindless heap exhausted at {} slots", capacity_);
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
} // namespace trishul::render::hardware
