// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/hardware/buffer_manager.h"
#include "engine/graphics/hardware/device.h"
#include "engine/graphics/hardware/fence.h"

#include "engine/utils/helpers.h"

#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include <spdlog/spdlog.h>

namespace cots::graphics::hardware
{
    buffer_manager::~buffer_manager()
    {
        deinitialize();
    }

    bool buffer_manager::initialize(device& dev)
    {
        device_ = &dev;
        slots_.reserve(64);

        //~ slot 0 reserved as invalid will be never handed out
        slots_.push_back(slot{});
        return device_->allocator() != nullptr;
    }

    void buffer_manager::deinitialize() noexcept
    {
        for (auto& s : slots_)
        {
            if (s.mapped && s.resource)
                s.resource->Unmap(0, nullptr);

            if (s.allocation)
                s.allocation->Release();   //~ releases the resource too
            s = slot{};
        }
        slots_.clear();
        device_ = nullptr;
    }

    std::uint32_t buffer_manager::acquire_slot()
    {
        for (std::uint32_t i = 1; i < slots_.size(); ++i)
        {
            const slot& s = slots_[i];

            //~ truly free slot
            if (s.generation == 0 &&
                s.resource == nullptr &&
                s.allocation == nullptr &&
                s.mapped == nullptr)
            {
                return i;
            }
        }

        slots_.push_back(slot{});
        return static_cast<std::uint32_t>(slots_.size() - 1);
    }

    buffer_manager::allocation_result
        buffer_manager::allocate_only(const buffer_create_info& info)
    {
        if (!device_ || !device_->allocator() || info.size_bytes == 0)
            return {};

        const std::uint32_t idx = acquire_slot();
        slot& s  = slots_[idx];
        s.generation = ~0u;
        s.size   = info.size_bytes;
        s.stride = info.stride;
        s.kind   = info.kind;

        const bool is_constant = (info.kind == buffer_kind::constant);

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = is_constant ? D3D12_HEAP_TYPE_UPLOAD
                                          : D3D12_HEAP_TYPE_DEFAULT;

        const std::uint64_t alloc_size = helpers::adjust_to_256(info.size_bytes, is_constant);

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment          = 0;
        desc.Width              = alloc_size;
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc         = { 1, 0 };
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        const D3D12_RESOURCE_STATES initial_state = is_constant
            ? D3D12_RESOURCE_STATE_GENERIC_READ
            : D3D12_RESOURCE_STATE_COMMON;

        if (FAILED(device_->allocator()->CreateResource(
                &alloc_desc, &desc, initial_state, nullptr,
                &s.allocation, IID_PPV_ARGS(&s.resource))))
        {
            spdlog::error("[buffer] CreateResource failed ({})", info.debug_name);
            s = slot{};
            return {};
        }

        wchar_t wname[64];
        swprintf_s(wname, L"%hs", info.debug_name);
        s.resource->SetName(wname);

        if (is_constant)
        {
            //~ persistently map
            constexpr D3D12_RANGE no_read{ 0, 0 };
            if (FAILED(s.resource->Map(0, &no_read, &s.mapped)))
            {
                spdlog::error("[buffer] CB Map failed ({})", info.debug_name);
                s.allocation->Release(); s = slot{};
                return {};
            }
            if (info.initial_data)
                std::memcpy(s.mapped, info.initial_data, info.size_bytes);
        }

        return { idx, true };
    }

    buffer_handle buffer_manager::create(const buffer_create_info& info)
    {
        const auto alloc = allocate_only(info);
        if (!alloc.ok) return buffer_handle::invalid();

        slot& s = slots_[alloc.index];

        //~ non constant with initial data uses the staged upload
        if (info.kind != buffer_kind::constant && info.initial_data)
        {
            if (!upload_static(s, info.initial_data, info.size_bytes))
            {
                s.allocation->Release(); s = slot{};
                return buffer_handle::invalid();
            }
        }

        const std::uint32_t gen = next_generation_++;
        if (next_generation_ == 0) next_generation_ = 1;
        s.generation = gen;

        return buffer_handle{ alloc.index, gen };
    }

    std::vector<buffer_handle>
        buffer_manager::create_batch(std::span<const buffer_create_info> infos)
    {
        std::vector<buffer_handle> out;
        out.reserve(infos.size());

        std::vector<std::uint32_t> alloc_indices;
        alloc_indices.reserve(infos.size());

        //~ pass one allocate every slot first
        //  slots vector may grow and invalidate pointers
        //  so do not capture slot pointers yet
        for (const auto& info : infos)
        {
            const auto a = allocate_only(info);
            if (!a.ok)
            {
                //~ roll back this batch
                for (const auto idx : alloc_indices)
                {
                    auto& s = slots_[idx];
                    if (s.mapped && s.resource) s.resource->Unmap(0, nullptr);
                    if (s.allocation) s.allocation->Release();
                    s = slot{};
                }
                return {};
            }
            alloc_indices.push_back(a.index);
        }

        //~ pass two build upload records
        //  slots is stable for the rest of this call
        std::vector<upload_record> uploads;
        uploads.reserve(infos.size());
        for (std::size_t i = 0; i < infos.size(); ++i)
        {
            const auto& info = infos[i];
            if (info.kind == buffer_kind::constant || !info.initial_data)
                continue;

            slot& s = slots_[alloc_indices[i]];
            upload_record r{};
            r.dst  = &s;
            r.data = info.initial_data;
            r.size = info.size_bytes;
            uploads.push_back(r);
        }

        //~ one staging one list one wait
        if (!uploads.empty())
        {
            if (!upload_batch(uploads))
            {
                spdlog::error("[buffer] batch upload failed");
                for (const auto idx : alloc_indices)
                {
                    auto& s = slots_[idx];
                    if (s.mapped && s.resource) s.resource->Unmap(0, nullptr);
                    if (s.allocation) s.allocation->Release();
                    s = slot{};
                }
                return {};
            }
        }

        //~ assign generations and produce handles
        for (const auto idx : alloc_indices)
        {
            slot& s = slots_[idx];
            const std::uint32_t gen = next_generation_++;
            if (next_generation_ == 0) next_generation_ = 1;
            s.generation = gen;
            out.push_back(buffer_handle{ idx, gen });
        }

        spdlog::info("[buffer] batch created {} buffers in one flush", out.size());
        return out;
    }

    //~ one staging arena
    //~ one command list
    //~ one fence wait
    bool buffer_manager::upload_batch(std::span<const upload_record> records) const
    {
        if (records.empty()) return true;

        auto* d3d   = device_->d3d12_device();
        auto* queue = device_->graphics_queue();
        auto* alloc = device_->allocator();

        //~ compute total staging size
        std::uint64_t total = 0;
        for (const auto& r : records) total += r.size;

        D3D12MA::ALLOCATION_DESC up_desc{};
        up_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

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
        if (FAILED(alloc->CreateResource(&up_desc, &ub,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                &staging_alloc, IID_PPV_ARGS(&staging))))
        {
            spdlog::error("[buffer] batch staging CreateResource failed");
            return false;
        }

        //~ map and copy all regions
        void* mapped = nullptr;
        constexpr D3D12_RANGE no_read{ 0, 0 };
        if (FAILED(staging->Map(0, &no_read, &mapped)))
        {
            spdlog::error("[buffer] batch staging Map failed");
            staging_alloc->Release();
            return false;
        }

        auto* base = static_cast<std::uint8_t*>(mapped);
        std::uint64_t off = 0;
        std::vector<std::uint64_t> offsets;
        offsets.reserve(records.size());
        for (const auto& r : records)
        {
            offsets.push_back(off);
            std::memcpy(base + off, r.data, r.size);
            off += r.size;
        }
        staging->Unmap(0, nullptr);

        //~ one shot command list
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    cmd_alloc;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd;
        d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmd_alloc));
        d3d->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_alloc.Get(),
                               nullptr, IID_PPV_ARGS(&cmd));

        //~ all copy regions
        for (std::size_t i = 0; i < records.size(); ++i)
        {
            cmd->CopyBufferRegion(records[i].dst->resource, 0,
                                  staging.Get(), offsets[i],
                                  records[i].size);
        }

        //~ all barriers in one call
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(records.size());
        for (const auto& r : records)
        {
            D3D12_RESOURCE_BARRIER b{};
            b.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition.pResource   = r.dst->resource;
            b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            b.Transition.StateAfter  =
                (r.dst->kind == buffer_kind::index)
                    ? D3D12_RESOURCE_STATE_INDEX_BUFFER
                    : D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers.push_back(b);
        }
        cmd->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

        cmd->Close();

        ID3D12CommandList* lists[] = { cmd.Get() };
        queue->ExecuteCommandLists(1, lists);

        //~ one wait
        fence flush_fence;
        if (!flush_fence.initialize(*device_))
        {
            staging_alloc->Release();
            return false;
        }
        const std::uint64_t target = flush_fence.signal(queue);
        (void)flush_fence.wait(target);
        flush_fence.deinitialize();

        staging_alloc->Release();
        return true;
    }

    //~ stage through an upload buffer copy on the graphics queue and block until done
    //  init-time only safe because no frames are in flight yet
    bool buffer_manager::upload_static(const slot& s, const void* data, const std::uint64_t size) const
    {
        auto* d3d   = device_->d3d12_device();
        auto* queue = device_->graphics_queue();
        auto* alloc = device_->allocator();

        //~ staging upload buffer
        D3D12MA::ALLOCATION_DESC up_desc{};
        up_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC ub{};
        ub.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        ub.Width            = size;
        ub.Height           = 1;
        ub.DepthOrArraySize = 1;
        ub.MipLevels        = 1;
        ub.Format           = DXGI_FORMAT_UNKNOWN;
        ub.SampleDesc       = { 1, 0 };
        ub.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12MA::Allocation* staging_alloc = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> staging;
        if (FAILED(alloc->CreateResource(&up_desc, &ub,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                &staging_alloc, IID_PPV_ARGS(&staging))))
        {
            spdlog::error("[buffer] staging CreateResource failed");
            return false;
        }

        //~ copy CPU data into staging
        void* mapped = nullptr;
        constexpr D3D12_RANGE no_read{ 0, 0 };
        if (FAILED(staging->Map(0, &no_read, &mapped)))
        {
            spdlog::error("[buffer] staging Map failed");
            staging_alloc->Release();
            return false;
        }
        std::memcpy(mapped, data, size);
        staging->Unmap(0, nullptr);

        //~ one-shot command list for the copy
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    cmd_alloc;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd;
        d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmd_alloc));
        d3d->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_alloc.Get(),
                               nullptr, IID_PPV_ARGS(&cmd));

        cmd->CopyBufferRegion(s.resource, 0, staging.Get(), 0, size);

        //~ transition default buffer common to appropriate read state
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = s.resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  =
            (s.kind == buffer_kind::index)
                ? D3D12_RESOURCE_STATE_INDEX_BUFFER
                : D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &barrier);

         (void)cmd->Close();

        ID3D12CommandList* lists[] = { cmd.Get() };
        queue->ExecuteCommandLists(1, lists);

        //~ block until the copy completes
        fence flush_fence;
        if (!flush_fence.initialize(*device_))
        {
            staging_alloc->Release(); return false;
        }

        const std::uint64_t target = flush_fence.signal(queue);
         (void)flush_fence.wait(target);
        flush_fence.deinitialize();

        staging_alloc->Release();
        return true;
    }

    void buffer_manager::destroy(const buffer_handle h)
    {
        if (!h.valid() || h.index >= slots_.size())
            return;

        slot& s = slots_[h.index];
        if (s.generation != h.generation)
            return;   //~ stale

        if (s.mapped && s.resource)
            s.resource->Unmap(0, nullptr);

        if (s.allocation)
            s.allocation->Release();
        s = slot{};
    }

    ID3D12Resource* buffer_manager::resource(const buffer_handle h) const
    {
        if (!h.valid() || h.index >= slots_.size())
            return nullptr;

        const slot& s = slots_[h.index];
        return s.generation == h.generation ? s.resource : nullptr;
    }

    std::uint64_t buffer_manager::gpu_address(const buffer_handle h) const
    {
        auto* r = resource(h);
        return r ? r->GetGPUVirtualAddress() : 0;
    }

    std::uint64_t buffer_manager::size(const buffer_handle h) const
    {
        if (!h.valid() || h.index >= slots_.size())
            return 0;

        const slot& s = slots_[h.index];
        return s.generation == h.generation ? s.size : 0;
    }

    std::uint64_t buffer_manager::stride(const buffer_handle h) const
    {
        if (!h.valid() || h.index >= slots_.size())
            return 0;

        const slot& s = slots_[h.index];
        return s.generation == h.generation ? s.stride : 0;
    }

    void* buffer_manager::mapped_ptr(const buffer_handle h) const
    {
        if (!h.valid() || h.index >= slots_.size())
            return nullptr;

        const slot& s = slots_[h.index];
        return s.generation == h.generation ? s.mapped : nullptr;
    }
} // namespace cots::graphics::hardware

