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
#include "trishul/renderer/hardware/texture_manager.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/renderer/hardware/descriptor_heap.h"
#include "trishul/renderer/hardware/deferred_releaser.h"
#include "trishul/renderer/hardware/upload_arena.h"
#include "trishul/utils/statics.h"
#include "trishul/core/engine_assert.h"

#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include <DirectXTex.h>
#include <cstring>
#include <string>
#include <vector>

#include "trishul/utils/logger.h"

namespace trishul::render::hardware
{
    namespace
    {
        DXGI_FORMAT to_dxgi(const texture_format f) noexcept
        {
            switch (f)
            {
            case texture_format::rgba8_unorm_srgb: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case texture_format::rgba8_unorm:
            default:                               return DXGI_FORMAT_R8G8B8A8_UNORM;
            }
        }

        std::uint32_t bytes_per_pixel(const texture_format f) noexcept
        {
            switch (f)
            {
            case texture_format::rgba8_unorm:
            case texture_format::rgba8_unorm_srgb:
            default:                               return 4u;
            }
        }
    } //~ anonymous namespace

    texture_manager::~texture_manager()
    {
        deinitialize();
    }

    bool texture_manager::initialize()
    {
        //~ wiring up the config for the 1st initialization
        if (!device_)
        {
            const auto* cfg = config_as<texture_manager_config>();
            ENGINE_ASSERT_MSG(cfg && cfg->dev && cfg->bindless,
                "texture_manager config missing call set_config<texture_manager_config> first");
            device_   = cfg->dev;
            bindless_ = cfg->bindless;
            if (cfg->releaser) releaser_ = cfg->releaser;
            if (cfg->arena)    arena_    = cfg->arena;
        }

        //~ up and running nobody flagged for rebuild
        if (!slots_.empty() && !need_rebuild_.load(std::memory_order_acquire))
            return true;

        //~ start with a clean table a device recreate throws away every gpu
        //~ texture so the game has to reload them we just hand back an empty set
        //~ TODO: Auto Reload resources later
        slots_.clear();
        slots_.reserve(32);
        slots_.push_back(slot{}); //~ slot zero is the reserved invalid handle

        need_rebuild_.store(false, std::memory_order_release);
        return device_->allocator() != nullptr;
    }

    void texture_manager::deinitialize() noexcept
    {
        //~ teardown happens with the gpu idle so we can free straight away with
        //~ no deferral slot zero falls out early since it never owns anything
        for (auto& s : slots_)
            release_slot_now(s, bindless_);

        slots_   .clear();
        device_   = nullptr;
        bindless_ = nullptr;
        releaser_ = nullptr;
        arena_    = nullptr;
        need_rebuild_.store(true, std::memory_order_release);
    }

    //~ frees everything one slot owns right now used on teardown and to clean
    //~ up a half built texture when create bails this is the inline path so the
    //~ just make sure the gpu is not still reading the texture
    void texture_manager::release_slot_now(slot& s, descriptor_heap* bindless) noexcept
    {
        //~ a live slot has a non zero generation an empty one owns nothing
        if (s.generation != 0u && bindless)
            bindless->release(s.bindless_slot);
        
        //~ reelease resources
        if (s.resource)   s.resource->Release();
        if (s.allocation) s.allocation->Release();

        s = slot{};
    }

    //~ find a free row in the table a freed slot has generation zero so we can
    //~ reuse it if none are free we grow start at one because slot zero is the
    //~ reserved invalid handle and must never gonna be handing it out
    std::uint32_t texture_manager::acquire_slot()
    {
        for (std::uint32_t i = 1; i < slots_.size(); ++i)
            if (slots_[i].generation == 0) return i;

        slots_.push_back(slot{});
        return static_cast<std::uint32_t>(slots_.size() - 1);
    }

    //~ the live rgba path make the gpu texture push the pixels up through the
    //~ arena grab a bindless srv then stamp a generation and hand back a handle
    //~ the generation will be catching any stale handle later if the slot got reused
    texture_handle texture_manager::create(const texture_create_info& info)
    {
        //~ bail early if we are not wired or the inputs make no sense
        if (!device_ || !device_->allocator() || !bindless_) return texture_handle::invalid();
        if (info.width == 0 || info.height == 0 || !info.pixels)
            return texture_handle::invalid();

        const std::uint32_t idx = acquire_slot();
        slot& s         = slots_[idx];
        s.width         = info.width;
        s.height        = info.height;
        s.mip_levels    = 1;
        s.dxgi_format   = to_dxgi(info.format);

        const DXGI_FORMAT dxgi = s.dxgi_format;

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment          = 0;
        desc.Width              = info.width;
        desc.Height             = info.height;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = dxgi;
        desc.SampleDesc         = { 1, 0 };
        desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        D3D12MA::Allocation* alloc = nullptr;
        ID3D12Resource2*     res   = nullptr;
        if (FAILED(device_->allocator()->CreateResource(
                &alloc_desc, &desc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                &alloc, IID_PPV_ARGS(&res))))
        {
            LOG_ERROR(".CreateResource failed ({})", info.debug_name);
            s = slot{};
            return texture_handle::invalid();
        }
        s.allocation = alloc;
        s.resource   = res;

        //~ name setup
        const std::wstring wname = info.debug_name
            ? statics::to_wide(info.debug_name)
            : std::wstring{};
        s.resource->SetName(wname.c_str());

        const std::uint32_t bpp        = bytes_per_pixel(info.format);
        const std::uint32_t tight_row  = info.width * bpp;
        const std::uint32_t source_row = info.row_pitch ? info.row_pitch : tight_row;

        if (!upload_pixels(s.resource, info.width, info.height, info.pixels, source_row))
        {
            //~ upload died throw away the resource we just made gen is still
            //~ zero so this only frees the resource and the allocation
            release_slot_now(s, bindless_);
            return texture_handle::invalid();
        }

        //~ grab a bindless slot
        s.bindless_slot = bindless_->acquire();
        if (s.bindless_slot == descriptor_heap::invalid_slot)
        {
            LOG_ERROR(".bindless slot acquire failed ({})", info.debug_name);
            //~ heap is full free the gpu resource the slot was never taken
            release_slot_now(s, bindless_);
            return texture_handle::invalid();
        }
        create_srv(s);

        const std::uint32_t gen = next_generation_++;
        if (next_generation_ == 0) next_generation_ = 1;
        s.generation = gen;

        LOG_INFO(".'{}' {}x{} created in bindless slot {}",
                     info.debug_name, info.width, info.height, s.bindless_slot);
        return texture_handle{ idx, gen };
    }

    //~ single mip 2d texture upload right now rides the shared arena pool
    // recycle keeps the staging allocation cost off from the hotspot or hotpath whatever
    bool texture_manager::upload_pixels(ID3D12Resource2* dst,
                                        const std::uint32_t width,
                                        const std::uint32_t height,
                                        const void* pixels,
                                        const std::uint32_t source_row) const
    {
        if (!arena_)
        {
            LOG_ERROR(".upload_pixels called before upload arena wired");
            return false;
        }
        if (!arena_->begin_batch())
        {
            LOG_ERROR(".arena begin_batch failed");
            return false;
        }
        if (!arena_->add_texture2d_copy(dst, width, height, pixels, source_row))
        {
            LOG_ERROR(".arena add_texture2d_copy failed");
            arena_->cancel_batch();
            return false;
        }
        return arena_->submit_and_wait();
    }

    //~ write the srv straight into the textures bindless slot the shader reads
    //~ it by that slot index this only does plain 2d textures a cube or an
    //~ array dds gonna be needing different view dimension here
    void texture_manager::create_srv(const slot& s) const
    {
        auto* d3d = device_->d3d12_device();

        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format                  = s.dxgi_format;
        srv.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels     = s.mip_levels;

        const D3D12_CPU_DESCRIPTOR_HANDLE cpu = bindless_->cpu_handle(s.bindless_slot);
        d3d->CreateShaderResourceView(s.resource, &srv, cpu);
    }

    void texture_manager::destroy(const texture_handle h)
    {
        if (!h.valid() || h.index >= slots_.size()) return;
        slot& s = slots_[h.index];
        if (s.generation != h.generation) return; //~ stale handle already freed

        if (releaser_) //~ the safe path push the frees behind the gpu fence so a
        {              //~ frame still reading this texture does not get it yanked
            releaser_->enqueue_bindless_slot(s.bindless_slot);
            releaser_->enqueue_allocation(s.allocation);

            //~ the resource ref we own goes through the queue
            //~ too attach takes our ref straight no extra addref performance boost
            if (s.resource)
            {
                Microsoft::WRL::ComPtr<ID3D12Resource2> res_com;
                res_com.Attach(s.resource);
                releaser_->enqueue_com(std::move(res_com));
            }
            s.allocation = nullptr;
            s.resource   = nullptr;
            s = slot{};
        }
        else //~ no releaser wired free inline and just hope the gpu is done
        {
            LOG_WARN("texture_manager destroy without a releaser freeing inline");
            release_slot_now(s, bindless_); //~ resets the slot for us
        }
    }

    ID3D12Resource2* texture_manager::resource(const texture_handle h) const
    {
        if (!h.valid() || h.index >= slots_.size()) return nullptr;
        const slot& s = slots_[h.index];
        return s.generation == h.generation ? s.resource : nullptr;
    }

    std::uint32_t texture_manager::bindless_slot(const texture_handle h) const
    {
        if (!h.valid() || h.index >= slots_.size()) return descriptor_heap::invalid_slot;
        const slot& s = slots_[h.index];
        return s.generation == h.generation ? s.bindless_slot : descriptor_heap::invalid_slot;
    }

    std::uint32_t texture_manager::width(const texture_handle h) const
    {
        if (!h.valid() || h.index >= slots_.size()) return 0;
        const slot& s = slots_[h.index];
        return s.generation == h.generation ? s.width : 0;
    }

    std::uint32_t texture_manager::height(const texture_handle h) const
    {
        if (!h.valid() || h.index >= slots_.size()) return 0;
        const slot& s = slots_[h.index];
        return s.generation == h.generation ? s.height : 0;
    }

    std::uint32_t texture_manager::mip_levels(const texture_handle h) const
    {
        if (!h.valid() || h.index >= slots_.size()) return 0;
        const slot& s = slots_[h.index];
        return s.generation == h.generation ? s.mip_levels : 0;
    }

    //~ the baked path same idea as the create but the pixels come from a dds blob
    //  directxtex parses it for us so we get the real format and the whole mip
    //  chain instead of a single rgba surface then it uploads every mip at once
    texture_handle texture_manager::create_from_dds(const dds_create_info& info)
    {
        if (!device_ || !device_->allocator() || !bindless_) return texture_handle::invalid();
        if (!info.dds_data || info.dds_size == 0)            return texture_handle::invalid();

        //~ parse the dds payload
        DirectX::TexMetadata  metadata{};
        DirectX::ScratchImage scratch{};
        if (const HRESULT hr = DirectX::LoadFromDDSMemory(
                info.dds_data, info.dds_size,
                DirectX::DDS_FLAGS_NONE, &metadata, scratch);
            FAILED(hr))
        {
            LOG_ERROR(".LoadFromDDSMemory failed for '{}' {:08X}",
                          info.debug_name, static_cast<std::uint32_t>(hr));
            return texture_handle::invalid();
        }

        const std::uint32_t idx = acquire_slot();
        slot& s       = slots_[idx];
        s.width       = static_cast<std::uint32_t>(metadata.width);
        s.height      = static_cast<std::uint32_t>(metadata.height);
        s.mip_levels  = static_cast<std::uint32_t>(metadata.mipLevels);
        s.dxgi_format = metadata.format;

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment          = 0;
        desc.Width              = metadata.width;
        desc.Height             = static_cast<UINT>(metadata.height);
        desc.DepthOrArraySize   = static_cast<UINT16>(metadata.arraySize);
        desc.MipLevels          = static_cast<UINT16>(metadata.mipLevels);
        desc.Format             = metadata.format;
        desc.SampleDesc         = { 1, 0 };
        desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        D3D12MA::Allocation* alloc = nullptr;
        ID3D12Resource2*     res   = nullptr;
        if (FAILED(device_->allocator()->CreateResource(
                &alloc_desc, &desc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                &alloc, IID_PPV_ARGS(&res))))
        {
            LOG_ERROR(".CreateResource failed for baked '{}'", info.debug_name);
            s = slot{};
            return texture_handle::invalid();
        }
        s.allocation = alloc;
        s.resource   = res;

        const std::wstring wname = info.debug_name
            ? statics::to_wide(info.debug_name)
            : std::wstring{};
        s.resource->SetName(wname.c_str());

        //~ upload all mips
        if (!upload_dds(s.resource, info.dds_data, info.dds_size, s.mip_levels))
        {
            //~ mip upload failed drop the resource gen is still zero
            release_slot_now(s, bindless_);
            return texture_handle::invalid();
        }

        //~ grab a bindless slot
        s.bindless_slot = bindless_->acquire();
        if (s.bindless_slot == descriptor_heap::invalid_slot)
        {
            LOG_ERROR(".bindless slot acquire failed for baked '{}'",
                          info.debug_name);
            //~ heap full free the gpu resource the slot was never taken
            release_slot_now(s, bindless_);
            return texture_handle::invalid();
        }
        create_srv(s);

        const std::uint32_t gen = next_generation_++;
        if (next_generation_ == 0) next_generation_ = 1;
        s.generation = gen;

        LOG_INFO(".'{}' baked {}x{} {} mips bindless slot {}",
                     info.debug_name, s.width, s.height, s.mip_levels, s.bindless_slot);
        return texture_handle{ idx, gen };
    }

    //~ dds mip chain upload routes the prepared subresources through the
    // arena one staging acquire one cmd list one fence wait for the whole
    // mip chain does not really care about mip count
    bool texture_manager::upload_dds(ID3D12Resource2* dst,
                                     const void* dds_data,
                                     const std::size_t dds_size,
                                     const std::uint32_t mip_count) const
    {
        if (!arena_)
        {
            LOG_ERROR(".upload_dds called before upload arena wired");
            return false;
        }

        //~ reparse for the mips
        DirectX::TexMetadata  metadata{};
        DirectX::ScratchImage scratch{};
        if (FAILED(DirectX::LoadFromDDSMemory(
                dds_data, dds_size,
                DirectX::DDS_FLAGS_NONE, &metadata, scratch)))
        {
            LOG_ERROR(".second parse failed during upload");
            return false;
        }

        //~ ask for the subresource layout
        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        if (FAILED(DirectX::PrepareUpload(
                device_->d3d12_device(),
                scratch.GetImages(), scratch.GetImageCount(),
                metadata,
                subresources)))
        {
            LOG_ERROR(".PrepareUpload failed");
            return false;
        }

        if (!arena_->begin_batch())
        {
            LOG_ERROR(".arena begin_batch failed for dds");
            return false;
        }
        if (!arena_->add_texture_subresources(dst, subresources))
        {
            LOG_ERROR(".arena add_texture_subresources failed");
            arena_->cancel_batch();
            return false;
        }

        (void)mip_count;
        return arena_->submit_and_wait();
    }
} // namespace trishul::render::hardware

