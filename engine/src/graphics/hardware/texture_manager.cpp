// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/hardware/texture_manager.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/hardware/descriptor_heap.h"
#include "engine/graphics/hardware/fence.h"

#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include <spdlog/spdlog.h>
#include <cstring>

namespace cots::graphics::hardware
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

    bool texture_manager::initialize(device& dev, descriptor_heap& bindless)
    {
        device_   = &dev;
        bindless_ = &bindless;
        slots_.reserve(32);

        //~ slot zero reserved as invalid
        slots_.push_back(slot{});

        return device_->allocator() != nullptr;
    }

    void texture_manager::deinitialize() noexcept
    {
        for (auto& s : slots_)
        {
            if (bindless_ && s.generation != 0)
                bindless_->release(s.bindless_slot);

            if (s.allocation)
                s.allocation->Release();

            s = slot{};
        }
        slots_   .clear();
        device_   = nullptr;
        bindless_ = nullptr;
    }

    std::uint32_t texture_manager::acquire_slot()
    {
        for (std::uint32_t i = 1; i < slots_.size(); ++i)
            if (slots_[i].generation == 0) return i;

        slots_.push_back(slot{});
        return static_cast<std::uint32_t>(slots_.size() - 1);
    }

    texture_handle texture_manager::create(const texture_create_info& info)
    {
        if (!device_ || !device_->allocator() || !bindless_) return texture_handle::invalid();
        if (info.width == 0 || info.height == 0 || !info.pixels)
            return texture_handle::invalid();

        const std::uint32_t idx = acquire_slot();
        slot& s    = slots_[idx];
        s.width    = info.width;
        s.height   = info.height;
        s.format   = info.format;

        const DXGI_FORMAT dxgi = to_dxgi(info.format);

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
            spdlog::error("[texture] CreateResource failed ({})", info.debug_name);
            s = slot{};
            return texture_handle::invalid();
        }
        s.allocation = alloc;
        s.resource   = res;

        wchar_t wname[64];
        swprintf_s(wname, L"%hs", info.debug_name);
        s.resource->SetName(wname);

        const std::uint32_t bpp        = bytes_per_pixel(info.format);
        const std::uint32_t tight_row  = info.width * bpp;
        const std::uint32_t source_row = info.row_pitch ? info.row_pitch : tight_row;

        if (!upload_pixels(s.resource, info.width, info.height, info.pixels, source_row))
        {
            alloc->Release();
            s = slot{};
            return texture_handle::invalid();
        }

        //~ grab a bindless slot
        s.bindless_slot = bindless_->acquire();
        if (s.bindless_slot == descriptor_heap::invalid_slot)
        {
            spdlog::error("[texture] bindless slot acquire failed ({})", info.debug_name);
            alloc->Release();
            s = slot{};
            return texture_handle::invalid();
        }
        create_srv(s);

        const std::uint32_t gen = next_generation_++;
        if (next_generation_ == 0) next_generation_ = 1;
        s.generation = gen;

        spdlog::info("[texture] '{}' {}x{} created in bindless slot {}",
                     info.debug_name, info.width, info.height, s.bindless_slot);
        return texture_handle{ idx, gen };
    }

    //~ stage and block
    //~ init time only
    bool texture_manager::upload_pixels(ID3D12Resource2* dst,
                                        const std::uint32_t width,
                                        const std::uint32_t height,
                                        const void* pixels,
                                        const std::uint32_t source_row) const
    {
        auto* d3d   = device_->d3d12_device();
        auto* queue = device_->graphics_queue();
        auto* alloc = device_->allocator();

        const D3D12_RESOURCE_DESC desc = dst->GetDesc();

        //~ ask for the upload layout
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        UINT   row_count = 0;
        UINT64 row_size  = 0;
        UINT64 total     = 0;
        d3d->GetCopyableFootprints(&desc, 0, 1, 0,
                                   &footprint, &row_count, &row_size, &total);

        //~ staging upload buffer
        D3D12MA::ALLOCATION_DESC up_alloc{};
        up_alloc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC ub{};
        ub.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        ub.Width            = total;
        ub.Height           = 1;
        ub.DepthOrArraySize = 1;
        ub.MipLevels        = 1;
        ub.Format           = DXGI_FORMAT_UNKNOWN;
        ub.SampleDesc       = { 1, 0 };
        ub.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12MA::Allocation* staging_alloc = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> staging;
        if (FAILED(alloc->CreateResource(&up_alloc, &ub,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                &staging_alloc, IID_PPV_ARGS(&staging))))
        {
            spdlog::error("[texture] staging CreateResource failed");
            return false;
        }

        //~ copy rows with destination pitch
        void* mapped = nullptr;
        constexpr D3D12_RANGE no_read{ 0, 0 };
        if (FAILED(staging->Map(0, &no_read, &mapped)))
        {
            spdlog::error("[texture] staging Map failed");
            staging_alloc->Release();
            return false;
        }

        auto*       dst_bytes = static_cast<std::uint8_t*>(mapped);
        const auto* src_bytes = static_cast<const std::uint8_t*>(pixels);
        const auto  dst_row   = static_cast<std::size_t>(footprint.Footprint.RowPitch);
        const auto  src_row   = static_cast<std::size_t>(source_row);
        const auto  copy_row  = static_cast<std::size_t>(row_size);

        for (std::uint32_t y = 0; y < height; ++y)
        {
            std::memcpy(dst_bytes + y * dst_row,
                        src_bytes + y * src_row,
                        copy_row);
        }
        staging->Unmap(0, nullptr);

        //~ one shot command list
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     cmd_alloc;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> cmd;
        d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmd_alloc));
        d3d->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_alloc.Get(),
                               nullptr, IID_PPV_ARGS(&cmd));

        //~ transition into copy dest
        constexpr D3D12_BARRIER_SUBRESOURCE_RANGE all_subresources{
            .IndexOrFirstMipLevel = 0xffffffffu,
            .NumMipLevels         = 0,
            .FirstArraySlice      = 0,
            .NumArraySlices       = 0,
            .FirstPlane           = 0,
            .NumPlanes            = 0,
        };

        D3D12_TEXTURE_BARRIER to_copy{};
        to_copy.SyncBefore   = D3D12_BARRIER_SYNC_NONE;
        to_copy.SyncAfter    = D3D12_BARRIER_SYNC_COPY;
        to_copy.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
        to_copy.AccessAfter  = D3D12_BARRIER_ACCESS_COPY_DEST;
        to_copy.LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED;
        to_copy.LayoutAfter  = D3D12_BARRIER_LAYOUT_COPY_DEST;
        to_copy.pResource    = dst;
        to_copy.Subresources = all_subresources;
        to_copy.Flags        = D3D12_TEXTURE_BARRIER_FLAG_DISCARD;

        D3D12_BARRIER_GROUP g0{
            .Type             = D3D12_BARRIER_TYPE_TEXTURE,
            .NumBarriers      = 1,
            .pTextureBarriers = &to_copy,
        };
        cmd->Barrier(1, &g0);

        D3D12_TEXTURE_COPY_LOCATION src_loc{};
        src_loc.pResource       = staging.Get();
        src_loc.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src_loc.PlacedFootprint = footprint;

        D3D12_TEXTURE_COPY_LOCATION dst_loc{};
        dst_loc.pResource        = dst;
        dst_loc.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst_loc.SubresourceIndex = 0;

        cmd->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, nullptr);

        //~ leave it in common
        D3D12_TEXTURE_BARRIER to_common{};
        to_common.SyncBefore   = D3D12_BARRIER_SYNC_COPY;
        to_common.SyncAfter    = D3D12_BARRIER_SYNC_NONE;
        to_common.AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
        to_common.AccessAfter  = D3D12_BARRIER_ACCESS_NO_ACCESS;
        to_common.LayoutBefore = D3D12_BARRIER_LAYOUT_COPY_DEST;
        to_common.LayoutAfter  = D3D12_BARRIER_LAYOUT_COMMON;
        to_common.pResource    = dst;
        to_common.Subresources = all_subresources;
        to_common.Flags        = D3D12_TEXTURE_BARRIER_FLAG_NONE;

        D3D12_BARRIER_GROUP g1{
            .Type             = D3D12_BARRIER_TYPE_TEXTURE,
            .NumBarriers      = 1,
            .pTextureBarriers = &to_common,
        };
        cmd->Barrier(1, &g1);

        cmd->Close();
        ID3D12CommandList* lists[] = { cmd.Get() };
        queue->ExecuteCommandLists(1, lists);

        fence flush_fence;
        if (!flush_fence.initialize(*device_)) { staging_alloc->Release(); return false; }
        const std::uint64_t target = flush_fence.signal(queue);
        flush_fence.wait(target);
        flush_fence.deinitialize();

        staging_alloc->Release();
        return true;
    }

    void texture_manager::create_srv(slot& s)
    {
        auto* d3d = device_->d3d12_device();

        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format                  = to_dxgi(s.format);
        srv.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels     = 1;

        const D3D12_CPU_DESCRIPTOR_HANDLE cpu = bindless_->cpu_handle(s.bindless_slot);
        d3d->CreateShaderResourceView(s.resource, &srv, cpu);
    }

    void texture_manager::destroy(const texture_handle h)
    {
        if (!h.valid() || h.index >= slots_.size()) return;
        slot& s = slots_[h.index];
        if (s.generation != h.generation) return;

        if (bindless_) bindless_->release(s.bindless_slot);
        if (s.allocation) s.allocation->Release();
        s = slot{};
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
} // namespace cots::graphics::hardware
