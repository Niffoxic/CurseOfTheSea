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
#include "trishul/renderer/hardware/upload_arena.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/utils/logger.h"
#include "trishul/core/engine_assert.h"

#include <D3D12MemAlloc.h>
#include <algorithm>
#include <cstring>

namespace trishul::render::hardware
{
    upload_arena::staging::staging(staging&& o) noexcept
        : allocation(o.allocation)
        , resource  (std::move(o.resource))
        , mapped    (o.mapped)
        , size      (o.size)
    {
        o.allocation = nullptr;
        o.mapped     = nullptr;
        o.size       = 0;
    }

    upload_arena::staging&
        upload_arena::staging::operator=(staging&& o) noexcept
    {
        if (this == &o) return *this;
        destroy();
        allocation = o.allocation; o.allocation = nullptr;
        resource   = std::move(o.resource);
        mapped     = o.mapped;     o.mapped     = nullptr;
        size       = o.size;       o.size       = 0;
        return *this;
    }

    upload_arena::staging::~staging() { destroy(); }

    void upload_arena::staging::destroy()
    {
        if (allocation)
        {
            allocation->Release();
            allocation = nullptr;
        }
        resource.Reset();
        mapped = nullptr;
        size   = 0;
    }

    upload_arena::~upload_arena() { deinitialize(); }

    bool upload_arena::initialize()
    {
        //~ wire the config
        if (!device_)
        {
            const auto* cfg = config_as<upload_arena_config>();
            ENGINE_ASSERT_MSG(cfg && cfg->dev,
                "upload_arena config missing call set_config<upload_arena_config> first");
            device_ = cfg->dev;
        }

        //~ already up and nobody flagged us nothing to do
        if (cmd_list_ && !need_rebuild_.load(std::memory_order_acquire)) return true;

        auto* d3d = device_->d3d12_device();
        if (!d3d)
        {
            LOG_ERROR("no device");
            return false;
        }

        //~ lives on the copy queue
        if (FAILED(d3d->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_COPY,
                IID_PPV_ARGS(&cmd_alloc_))))
        {
            LOG_ERROR("CreateCommandAllocator failed");
            return false;
        }
        (void)cmd_alloc_->SetName(L"upload_arena cmd alloc");

        if (FAILED(d3d->CreateCommandList(
                0, D3D12_COMMAND_LIST_TYPE_COPY,
                cmd_alloc_.Get(), nullptr,
                IID_PPV_ARGS(&cmd_list_))))
        {
            LOG_ERROR("CreateCommandList failed");
            return false;
        }
        (void)cmd_list_->SetName(L"upload_arena cmd list");
        //~ close immediately begin_batch resets before recording
        (void)cmd_list_->Close();

        if (FAILED(d3d->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                    IID_PPV_ARGS(&fence_))))
        {
            LOG_ERROR("CreateFence failed");
            return false;
        }
        (void)fence_->SetName(L"upload_arena fence");

        next_signal_.store(1u, std::memory_order_release);
        batch_open_ = false;
        need_rebuild_.store(false, std::memory_order_release);
        LOG_INFO("upload arena ready");
        return true;
    }

    void upload_arena::deinitialize() noexcept
    {
        //~ shutdown is post gpu idle so the pool is safe to drain
        free_pool_ .clear();
        in_use_    .clear();
        in_flight_ .clear();

        buffer_copies_     .clear();
        texture_copies_    .clear();
        pre_tex_barriers_  .clear();
        post_tex_barriers_ .clear();
        post_buf_barriers_ .clear();

        cmd_list_  .Reset();
        cmd_alloc_ .Reset();
        fence_     .Reset();

        reused_     = 0;
        allocated_  = 0;
        batch_open_ = false;
        device_     = nullptr;
        need_rebuild_.store(true, std::memory_order_release); //~ reusable
    }

    bool upload_arena::begin_batch()
    {
        if (batch_open_)
        {
            LOG_WARN("begin_batch called with batch already open");
            return false;
        }
        if (!device_ || !cmd_alloc_ || !cmd_list_)
        {
            LOG_ERROR("begin_batch before init");
            return false;
        }

        //~ reclaim everything the gpu is done with
        recycle_completed();

        if (FAILED(cmd_alloc_->Reset()))
        {
            LOG_ERROR("cmd allocator reset failed");
            return false;
        }
        if (FAILED(cmd_list_->Reset(cmd_alloc_.Get(), nullptr)))
        {
            LOG_ERROR("cmd list reset failed");
            return false;
        }

        buffer_copies_     .clear();
        texture_copies_    .clear();
        pre_tex_barriers_  .clear();
        post_tex_barriers_ .clear();
        post_buf_barriers_ .clear();

        batch_open_ = true;
        return true;
    }

    bool upload_arena::allocate_staging(const std::uint64_t size, staging& out)
    {
        auto* allocator = device_->allocator();
        if (!allocator)
        {
            LOG_ERROR("no allocator");
            return false;
        }

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC ub{};
        ub.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        ub.Width            = size;
        ub.Height           = 1;
        ub.DepthOrArraySize = 1;
        ub.MipLevels        = 1;
        ub.Format           = DXGI_FORMAT_UNKNOWN;
        ub.SampleDesc       = { 1, 0 };
        ub.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12MA::Allocation*                    alloc = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource>  res;
        if (FAILED(allocator->CreateResource(
                &alloc_desc, &ub,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                &alloc, IID_PPV_ARGS(&res))))
        {
            LOG_ERROR("staging CreateResource failed for {} bytes", size);
            return false;
        }
        (void)res->SetName(L"upload_arena staging");

        //~ persistently map the upload heap reads back are not legal here
        constexpr D3D12_RANGE no_read{ 0, 0 };
        void* mapped = nullptr;
        if (FAILED(res->Map(0, &no_read, &mapped)))
        {
            LOG_ERROR("staging Map failed");
            alloc->Release();
            return false;
        }

        out.allocation = alloc;
        out.resource   = std::move(res);
        out.mapped     = static_cast<std::uint8_t*>(mapped);
        out.size       = size;
        ++allocated_;
        return true;
    }

    bool upload_arena::acquire_staging(const std::uint64_t size, staging& out)
    {
        //~ first fit walk the free pool early sizes are typically uniform
        // optimizes later when the pool grows wide
        for (std::size_t i = 0; i < free_pool_.size(); ++i)
        {
            if (free_pool_[i].size >= size)
            {
                out = std::move(free_pool_[i]);
                free_pool_.erase(free_pool_.begin() + static_cast<std::ptrdiff_t>(i));
                ++reused_;
                return true;
            }
        }
        return allocate_staging(size, out);
    }

    bool upload_arena::add_buffer_copy(ID3D12Resource* dst,
                                       const void* data,
                                       const std::uint64_t size,
                                       const D3D12_RESOURCE_STATES final_state)
    {
        if (!batch_open_ || !dst || !data || size == 0u) return false;

        staging s;
        if (!acquire_staging(size, s)) return false;

        std::memcpy(s.mapped, data, size);

        buffer_copy_record rec{};
        rec.dst              = dst;
        rec.staging_resource = s.resource.Get();
        rec.staging_offset   = 0;
        rec.size             = size;
        buffer_copies_.push_back(rec);

        buf_post_barrier_record br{};
        br.buffer = dst;
        br.after  = final_state;
        post_buf_barriers_.push_back(br);

        in_use_.push_back(std::move(s));
        return true;
    }

    bool upload_arena::add_texture2d_copy(ID3D12Resource* dst,
                                          const std::uint32_t width,
                                          const std::uint32_t height,
                                          const void* pixels,
                                          const std::uint32_t source_row_pitch)
    {
        if (!batch_open_ || !dst || !pixels || width == 0u || height == 0u) return false;

        const D3D12_RESOURCE_DESC desc = dst->GetDesc();

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        UINT   row_count = 0;
        UINT64 row_size  = 0;
        UINT64 total     = 0;
        device_->d3d12_device()->GetCopyableFootprints(
            &desc, 0, 1, 0,
            &footprint, &row_count, &row_size, &total);

        staging s;
        if (!acquire_staging(total, s)) return false;

        //~ copy rows respecting both source and destination pitch
        const auto dst_row = static_cast<std::size_t>(footprint.Footprint.RowPitch);
        const auto src_row = static_cast<std::size_t>(source_row_pitch ? source_row_pitch
                                                                       : row_size);
        const auto copy_row = static_cast<std::size_t>(row_size);
        const auto* src_bytes = static_cast<const std::uint8_t*>(pixels);
        for (std::uint32_t y = 0; y < height; ++y)
        {
            std::memcpy(s.mapped + y * dst_row,
                        src_bytes + y * src_row,
                        copy_row);
        }

        //~ queue the layout transition and the copy command
        pre_tex_barriers_ .push_back({ dst });
        texture_copies_   .push_back({});
        auto& tc = texture_copies_.back();
        tc.src.pResource       = s.resource.Get();
        tc.src.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        tc.src.PlacedFootprint = footprint;
        tc.dst.pResource        = dst;
        tc.dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        tc.dst.SubresourceIndex = 0;
        post_tex_barriers_.push_back({ dst });

        in_use_.push_back(std::move(s));
        return true;
    }

    bool upload_arena::add_texture_subresources(
        ID3D12Resource* dst,
        const std::span<const D3D12_SUBRESOURCE_DATA> subresources)
    {
        if (!batch_open_ || !dst || subresources.empty()) return false;

        const D3D12_RESOURCE_DESC desc = dst->GetDesc();
        const UINT num = static_cast<UINT>(subresources.size());

        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(num);
        std::vector<UINT>   row_counts(num);
        std::vector<UINT64> row_sizes (num);
        UINT64 total = 0;
        device_->d3d12_device()->GetCopyableFootprints(
            &desc, 0, num, 0,
            footprints.data(),
            row_counts.data(),
            row_sizes.data(),
            &total);

        staging s;
        if (!acquire_staging(total, s)) return false;

        //~ copy every subresource into staging at its placed footprint
        for (UINT i = 0; i < num; ++i)
        {
            const auto& fp  = footprints[i];
            const auto& sub = subresources[i];
            const auto dst_row = static_cast<std::size_t>(fp.Footprint.RowPitch);
            const auto src_row = static_cast<std::size_t>(sub.RowPitch);
            const auto copy_row = static_cast<std::size_t>(row_sizes[i]);
            const UINT rows = row_counts[i];
            const UINT depth = fp.Footprint.Depth;

            auto*       dst_base = s.mapped + fp.Offset;
            const auto* src_base = static_cast<const std::uint8_t*>(sub.pData);

            //~ depth loop handles 3d textures and arrays through slice pitch
            for (UINT z = 0; z < depth; ++z)
            {
                auto* dst_slice = dst_base + z * dst_row * rows;
                const auto* src_slice = src_base + z * sub.SlicePitch;
                for (UINT r = 0; r < rows; ++r)
                {
                    std::memcpy(dst_slice + r * dst_row,
                                src_slice + r * src_row,
                                copy_row);
                }
            }
        }

        //~ single pre and post barrier covers all subresources
        pre_tex_barriers_ .push_back({ dst });
        for (UINT i = 0; i < num; ++i)
        {
            texture_copies_.push_back({});
            auto& tc = texture_copies_.back();
            tc.src.pResource       = s.resource.Get();
            tc.src.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            tc.src.PlacedFootprint = footprints[i];
            tc.dst.pResource        = dst;
            tc.dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            tc.dst.SubresourceIndex = i;
        }
        post_tex_barriers_.push_back({ dst });

        in_use_.push_back(std::move(s));
        return true;
    }

    upload_arena::fence_value upload_arena::submit()
    {
        if (!batch_open_)
        {
            LOG_WARN("submit called without begin_batch");
            return 0u;
        }

        //~ copy queue texture barriers
        //~ record the copies buffers first then textures
        for (const auto& bc : buffer_copies_)
        {
            cmd_list_->CopyBufferRegion(bc.dst, 0,
                                        bc.staging_resource,
                                        bc.staging_offset,
                                        bc.size);
        }
        for (const auto& tc : texture_copies_)
        {
            cmd_list_->CopyTextureRegion(&tc.dst, 0, 0, 0, &tc.src, nullptr);
        }

        //~ post copy buffer transitions on the copy queue
        if (!post_buf_barriers_.empty())
        {
            std::vector<D3D12_RESOURCE_BARRIER> bs;
            bs.reserve(post_buf_barriers_.size());
            for (const auto& p : post_buf_barriers_)
            {
                D3D12_RESOURCE_BARRIER b{};
                b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                b.Transition.pResource   = p.buffer;
                b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                b.Transition.StateAfter  = D3D12_RESOURCE_STATE_COMMON;
                b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                bs.push_back(b);
            }
            cmd_list_->ResourceBarrier(static_cast<UINT>(bs.size()), bs.data());
        }

        if (FAILED(cmd_list_->Close()))
        {
            LOG_ERROR("cmd list Close failed");
            batch_open_ = false;
            return 0u;
        }

        //~ lives on the copy queue the arena owns its own fence
        // so this signal does not collide with the per
        // queue timelines
        ID3D12CommandList* lists[] = { cmd_list_.Get() };
        auto* submit_queue = device_->copy_queue();
        if (!submit_queue) submit_queue = device_->graphics_queue();
        submit_queue->ExecuteCommandLists(1, lists);

        const fence_value target = next_signal_.fetch_add(1u, std::memory_order_acq_rel);
        if (FAILED(submit_queue->Signal(fence_.Get(), target)))
        {
            LOG_ERROR("queue Signal failed");
            batch_open_ = false;
            return 0u;
        }

        //~ move every staging buffer used this batch into the in flight
        // list stamped with the value the gpu will reach once done
        in_flight_.reserve(in_flight_.size() + in_use_.size());
        for (auto& b : in_use_)
        {
            in_flight_.push_back({ std::move(b), target });
        }
        in_use_.clear();

        batch_open_ = false;
        return target;
    }

    bool upload_arena::wait(const fence_value v) const
    {
        if (!fence_) return false;
        if (fence_->GetCompletedValue() >= v) return true;

        //~ per call event so two loader threads can wait at the same time
        // without racing on a single shared handle
        HANDLE ev = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!ev)
        {
            LOG_ERROR("CreateEvent failed in wait");
            return false;
        }
        if (FAILED(fence_->SetEventOnCompletion(v, ev)))
        {
            LOG_ERROR("SetEventOnCompletion failed");
            ::CloseHandle(ev);
            return false;
        }
        ::WaitForSingleObject(ev, INFINITE);
        ::CloseHandle(ev);
        return true;
    }

    bool upload_arena::is_complete(const fence_value v) const
    {
        return fence_ && fence_->GetCompletedValue() >= v;
    }

    bool upload_arena::submit_and_wait()
    {
        const fence_value f = submit();
        if (f == 0u) return false;
        const bool ok = wait(f);
        if (ok) recycle_completed();
        return ok;
    }

    void upload_arena::cancel_batch()
    {
        if (!batch_open_) return;
        //~ throw the queued staging back into the free pool no gpu work
        // recorded on the cmd list so we can just close and ignore it
        for (auto& b : in_use_) free_pool_.push_back(std::move(b));
        in_use_.clear();

        buffer_copies_     .clear();
        texture_copies_    .clear();
        pre_tex_barriers_  .clear();
        post_tex_barriers_ .clear();
        post_buf_barriers_ .clear();

        //~ closing it now so that next reset dont cry or needs extra checks
        if (cmd_list_)
        {
            if (FAILED(cmd_list_->Close()))
            {
                LOG_WARN("cancel_batch Close failed");
            }
        }
        batch_open_ = false;
    }

    std::size_t upload_arena::recycle_completed()
    {
        if (!fence_) return 0u;
        const fence_value done = fence_->GetCompletedValue();

        std::size_t reclaimed = 0u;
        for (auto it = in_flight_.begin(); it != in_flight_.end();)
        {
            if (it->stamp <= done)
            {
                free_pool_.push_back(std::move(it->buf));
                it = in_flight_.erase(it);
                ++reclaimed;
            }
            else
            {
                ++it;
            }
        }
        return reclaimed;
    }

    std::size_t upload_arena::free_count() const noexcept
    {
        return free_pool_.size();
    }

    std::size_t upload_arena::in_flight_count() const noexcept
    {
        return in_flight_.size();
    }
} // namespace trishul::render::hardware
