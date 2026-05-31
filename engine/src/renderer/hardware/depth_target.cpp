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
#include "trishul/renderer/hardware/depth_target.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/renderer/hardware/descriptor_heap.h"
#include "trishul/utils/logger.h"
#include "trishul/core/engine_assert.h"

#include <d3d12.h>
#include <D3D12MemAlloc.h>

namespace trishul::render::hardware
{
    depth_target::~depth_target()
    {
        deinitialize();
    }

    bool depth_target::initialize()
    {
        if (!device_)
        {
            const auto* cfg = config_as<depth_target_config>();
            ENGINE_ASSERT_MSG(cfg && cfg->dev,
                "depth_target config missing call set_config<depth_target_config> first");
            device_   = cfg->dev;
            bindless_ = cfg->bindless;
            width_    = cfg->width  > 0u ? cfg->width  : 1u;
            height_   = cfg->height > 0u ? cfg->height : 1u;
        }

        //~ already up and nobody flagged it nothing to do
        if (resource_ && !need_rebuild_.load(std::memory_order_acquire))
            return true;

        release_resource();
        dsv_heap_.Reset();
        srv_slot_ = invalid_slot;

        if (!build()) return false;

        need_rebuild_.store(false, std::memory_order_release);
        return true;
    }

    bool depth_target::build()
    {
        if (!create_dsv_heap()) return false;
        if (!create_resource()) return false;
        if (!create_dsv())      return false;
        if (!create_srv())      return false;

        LOG_INFO("depth target {}x{} (srv slot {})", width_, height_, srv_slot_);
        return true;
    }

    bool depth_target::create_dsv_heap()
    {
        if (dsv_heap_) return true; //~ resize reuses the existing heap

        auto* d3d = device_->d3d12_device();
        if (!d3d)
        {
            LOG_ERROR("device not initialized");
            return false;
        }

        //~ small non shader visible dsv heap with one slot
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heap_desc.NumDescriptors = 1;
        heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        if (FAILED(d3d->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&dsv_heap_))))
        {
            LOG_ERROR("CreateDescriptorHeap (DSV) failed");
            return false;
        }
        (void)dsv_heap_->SetName(L"COTS DSV Heap");
        return true;
    }

    bool depth_target::create_resource()
    {
        if (!device_->allocator())
        {
            LOG_ERROR("allocator not initialized");
            return false;
        }

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        //~ typeless backing so the same texture serves a dsv and an srv
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment        = 0;
        desc.Width            = width_;
        desc.Height           = height_;
        desc.DepthOrArraySize = 1;
        desc.MipLevels        = 1;
        desc.Format           = resource_format;
        desc.SampleDesc       = { 1, 0 };
        desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        //~ reversed Z optimized clear depth 0 stencil 0 must use the typed dsv format
        D3D12_CLEAR_VALUE clear_value{};
        clear_value.Format               = dsv_format;
        clear_value.DepthStencil.Depth   = 0.0f;
        clear_value.DepthStencil.Stencil = 0;

        D3D12MA::Allocation* alloc = nullptr;
        if (FAILED(device_->allocator()->CreateResource(
                &alloc_desc, &desc,
                D3D12_RESOURCE_STATE_COMMON,
                &clear_value,
                &alloc, IID_PPV_ARGS(&resource_))))
        {
            LOG_ERROR("CreateResource (depth) failed");
            return false;
        }
        allocation_ = alloc;
        (void)resource_->SetName(L"COTS Depth Target");
        return true;
    }

    bool depth_target::create_dsv() const
    {
        auto* d3d = device_->d3d12_device();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
        dsv_desc.Format        = dsv_format;
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Flags         = D3D12_DSV_FLAG_NONE;

        d3d->CreateDepthStencilView(
            resource_.Get(), &dsv_desc,
            dsv_heap_->GetCPUDescriptorHandleForHeapStart());
        return true;
    }

    bool depth_target::create_srv()
    {
        if (!bindless_) return true; //~ no bindless heap the srv is optional

        //~ grab a slot only if we dont already hold one
        if (srv_slot_ == invalid_slot)
        {
            srv_slot_ = bindless_->acquire();
            if (srv_slot_ == descriptor_heap::invalid_slot)
            {
                srv_slot_ = invalid_slot;
                LOG_ERROR("bindless heap full no slot for depth srv");
                return false;
            }
        }

        //~ read the depth component ignore the stencil bits
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format                  = srv_format;
        srv.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels     = 1;

        device_->d3d12_device()->CreateShaderResourceView(
            resource_.Get(), &srv, bindless_->cpu_handle(srv_slot_));
        return true;
    }

    bool depth_target::resize(const std::uint32_t width, const std::uint32_t height)
    {
        if (!device_)
        {
            LOG_ERROR("depth_target resize before configure");
            return false;
        }
        if (!resource_) return initialize(); //~ not built yet

        //~ same device and heap keep the bindless slot remake resource and views
        //~ gotta make sure gpu is idled the resources and safe to free (flight check)
        release_resource();

        width_  = width  > 0u ? width  : 1u;
        height_ = height > 0u ? height : 1u;

        if (!create_resource()) return false;
        if (!create_dsv())      return false;
        if (!create_srv())      return false; //~ srv_slot_ still valid reused

        LOG_INFO("depth target resized {}x{}", width_, height_);
        return true;
    }

    void depth_target::deinitialize() noexcept
    {
        release_srv();
        release_resource();
        dsv_heap_.Reset();

        width_    = 1u;
        height_   = 1u;
        device_   = nullptr;
        bindless_ = nullptr;
        need_rebuild_.store(true, std::memory_order_release); //~ reusable
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

    void depth_target::release_srv() noexcept
    {
        if (bindless_ && srv_slot_ != invalid_slot)
            bindless_->release(srv_slot_);
        srv_slot_ = invalid_slot;
    }

    ID3D12Resource2* depth_target::resource() const noexcept
    {
        return resource_.Get();
    }

    std::size_t depth_target::dsv_handle() const noexcept
    {
        if (!dsv_heap_) return 0;
        return dsv_heap_->GetCPUDescriptorHandleForHeapStart().ptr;
    }
} // namespace trishul::render::hardware
