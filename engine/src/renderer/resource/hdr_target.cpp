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
#include "trishul/renderer/resource/hdr_target.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/renderer/hardware/descriptor_heap.h"
#include "trishul/renderer/clear_color.h"
#include "trishul/utils/logger.h"

#include <d3d12.h>
#include <D3D12MemAlloc.h>

namespace trishul::render::resource
{
    hdr_target::~hdr_target()
    {
        deinitialize();
    }

    bool hdr_target::initialize()
    {
        if (resource_) return true;

        const auto* cfg = config_as<hdr_target_config>();
        if (!cfg || !cfg->dev || !cfg->bindless)
        {
            LOG_ERROR("hdr_target missing config device or bindless heap");
            return false;
        }

        auto* d3d = cfg->dev->d3d12_device();
        if (!d3d || !cfg->dev->allocator())
        {
            LOG_ERROR("hdr_target device or allocator not initialized");
            return false;
        }

        device_   = cfg->dev;
        bindless_ = cfg->bindless;
        width_    = cfg->width  > 0u ? cfg->width  : 1u;
        height_   = cfg->height > 0u ? cfg->height : 1u;

        //~ a single non shader visible rtv just for this target
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heap_desc.NumDescriptors = 1;
        heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        if (FAILED(d3d->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heap_))))
        {
            LOG_ERROR("hdr_target CreateDescriptorHeap rtv failed");
            return false;
        }
        (void)rtv_heap_->SetName(L"COTS HDR RTV Heap");

        srv_slot_ = bindless_->acquire();
        if (srv_slot_ == hardware::descriptor_heap::invalid_slot)
        {
            LOG_ERROR("hdr_target bindless slot acquire failed");
            return false;
        }

        if (!create_resource()) return false;

        need_rebuild_.store(false, std::memory_order_release);
        return true;
    }

    void hdr_target::deinitialize() noexcept
    {
        release_resource();
        rtv_heap_.Reset();
        if (bindless_ && srv_slot_ != hardware::descriptor_heap::invalid_slot)
        {
            bindless_->release(srv_slot_);
        }
        srv_slot_ = hardware::descriptor_heap::invalid_slot;
        bindless_ = nullptr;
        device_   = nullptr;
        width_    = 1u;
        height_   = 1u;
    }

    bool hdr_target::resize(const std::uint32_t width, const std::uint32_t height)
    {
        //~ nothing stood up yet so a resize is really just a first build
        if (!rtv_heap_ || !device_) return false;

        release_resource();
        width_  = width  > 0u ? width  : 1u;
        height_ = height > 0u ? height : 1u;
        return create_resource();
    }

    bool hdr_target::create_resource()
    {
        auto* d3d = device_->d3d12_device();

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment        = 0;
        desc.Width            = width_;
        desc.Height           = height_;
        desc.DepthOrArraySize = 1;
        desc.MipLevels        = 1;
        desc.Format           = k_format;
        desc.SampleDesc       = { 1, 0 };
        desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        //~ the clear color never changes so baking it
        D3D12_CLEAR_VALUE optimized{};
        optimized.Format   = k_format;
        optimized.Color[0] = k_clear_color[0];
        optimized.Color[1] = k_clear_color[1];
        optimized.Color[2] = k_clear_color[2];
        optimized.Color[3] = k_clear_color[3];

        D3D12MA::Allocation* alloc = nullptr;
        if (FAILED(device_->allocator()->CreateResource(
                &alloc_desc, &desc,
                D3D12_RESOURCE_STATE_COMMON,
                &optimized,
                &alloc, IID_PPV_ARGS(&resource_))))
        {
            LOG_ERROR("hdr_target CreateResource failed");
            return false;
        }
        allocation_ = alloc;
        (void)resource_->SetName(L"COTS HDR Color Target");

        //~ rtv so the forward passes can render into it
        D3D12_RENDER_TARGET_VIEW_DESC rtv{};
        rtv.Format        = k_format;
        rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        d3d->CreateRenderTargetView(
            resource_.Get(), &rtv,
            rtv_heap_->GetCPUDescriptorHandleForHeapStart());

        //~ srv into the bindless heap so tonemap and post can sample it later
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format                  = k_format;
        srv.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels     = 1;

        const auto cpu = bindless_->cpu_handle(srv_slot_);
        d3d->CreateShaderResourceView(resource_.Get(), &srv, cpu);

        LOG_INFO("hdr_target created {}x{} bindless slot {}", width_, height_, srv_slot_);
        return true;
    }

    void hdr_target::release_resource() noexcept
    {
        resource_.Reset();
        if (allocation_)
        {
            allocation_->Release();
            allocation_ = nullptr;
        }
    }

    ID3D12Resource2* hdr_target::resource() const noexcept
    {
        return resource_.Get();
    }

    std::size_t hdr_target::rtv_handle() const noexcept
    {
        if (!rtv_heap_) return 0;
        return rtv_heap_->GetCPUDescriptorHandleForHeapStart().ptr;
    }
} // namespace trishul::render::resource
