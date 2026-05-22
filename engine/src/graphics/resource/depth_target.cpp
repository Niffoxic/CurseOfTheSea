// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/resource/depth_target.h"
#include "engine/graphics/hardware/device.h"

#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include <spdlog/spdlog.h>

namespace cots::graphics::resource
{
    namespace
    {
        constexpr DXGI_FORMAT k_depth_format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    }

    depth_target::~depth_target()
    {
        deinitialize();
    }

    bool depth_target::initialize(const hardware::device& dev,
                                  const std::uint32_t width,
                                  const std::uint32_t height)
    {
        if (resource_) return true;

        auto* d3d = dev.d3d12_device();
        if (!d3d || !dev.allocator())
        {
            spdlog::error("[hardware:depth] device or allocator not initialized");
            return false;
        }

        width_  = width  > 0 ? width  : 1;
        height_ = height > 0 ? height : 1;

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heap_desc.NumDescriptors = 1;
        heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        if (FAILED(d3d->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&dsv_heap_))))
        {
            spdlog::error("[hardware:depth] CreateDescriptorHeap (DSV) failed");
            return false;
        }
        dsv_heap_->SetName(L"COTS DSV Heap");

        return create_resource(dev);
    }

    void depth_target::deinitialize() noexcept
    {
        release_resource();
        dsv_heap_.Reset();
        width_  = 0;
        height_ = 0;
    }

    bool depth_target::resize(const hardware::device& dev,
                              const std::uint32_t width,
                              const std::uint32_t height)
    {
        if (!dsv_heap_)
        {
            return initialize(dev, width, height);
        }

        release_resource();

        width_  = width  > 0 ? width  : 1;
        height_ = height > 0 ? height : 1;

        return create_resource(dev);
    }

    bool depth_target::create_resource(const hardware::device& dev)
    {
        auto* d3d = dev.d3d12_device();

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment          = 0;
        desc.Width              = width_;
        desc.Height             = height_;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = k_depth_format;
        desc.SampleDesc         = { 1, 0 };
        desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        //~ reversed z for optimized clear deapth
        D3D12_CLEAR_VALUE clear_value{};
        clear_value.Format               = k_depth_format;
        clear_value.DepthStencil.Depth   = 0.0f;
        clear_value.DepthStencil.Stencil = 0;

        D3D12MA::Allocation* alloc = nullptr;
        if (FAILED(dev.allocator()->CreateResource(
                &alloc_desc, &desc,
                D3D12_RESOURCE_STATE_COMMON,
                &clear_value,
                &alloc, IID_PPV_ARGS(&resource_))))
        {
            spdlog::error("[hardware:depth] CreateResource failed");
            return false;
        }
        allocation_ = alloc;
        resource_->SetName(L"COTS Depth Target");

        //~ dsv
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
        dsv_desc.Format        = k_depth_format;
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Flags         = D3D12_DSV_FLAG_NONE;

        const D3D12_CPU_DESCRIPTOR_HANDLE handle =
            dsv_heap_->GetCPUDescriptorHandleForHeapStart();
        d3d->CreateDepthStencilView(resource_.Get(), &dsv_desc, handle);

        spdlog::info("[hardware:depth] created {}x{}", width_, height_);
        return true;
    }

    void depth_target::release_resource() noexcept
    {
        resource_.Reset();
        if (allocation_)
        {
            allocation_->Release();
            allocation_ = nullptr;
        }
    }

    ID3D12Resource2* depth_target::resource() const noexcept
    {
        return resource_.Get();
    }

    std::size_t depth_target::dsv_handle() const noexcept
    {
        if (!dsv_heap_) return 0;
        const auto [ptr] = dsv_heap_->GetCPUDescriptorHandleForHeapStart();
        return ptr;
    }
} // namespace cots::graphics::hardware
