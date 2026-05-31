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
#include "trishul/renderer/hardware/buffer_manager.h"
#include "trishul/renderer/hardware/device.h"
#include "trishul/renderer/hardware/deferred_releaser.h"
#include "trishul/renderer/hardware/upload_arena.h"

#include "trishul/utils/statics.h"
#include "trishul/utils/logger.h"
#include "trishul/core/engine_assert.h"

#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include <cstring>
#include <string>

namespace trishul::render::hardware
{
    buffer_manager::~buffer_manager()
    {
        deinitialize();
    }

    bool buffer_manager::initialize()
    {
        //~ grabs the wiring the handler set up
        if (!device_)
        {
            const auto* cfg = config_as<buffer_manager_config>();
            ENGINE_ASSERT_MSG(cfg && cfg->dev,
                "buffer_manager config missing call set_config<buffer_manager_config> first");
            device_ = cfg->dev;
            if (cfg->releaser) releaser_ = cfg->releaser;
            if (cfg->arena)    arena_    = cfg->arena;
        }

        //~ already up nobody flagged a rebuild
        if (!need_rebuild_.load(std::memory_order_acquire))
            return true;

        //~ fresh table a device recreate throws away every buffer the game
        //~ remakes them we just start empty TODO auto reload later
        buffers_.clear();

        need_rebuild_.store(false, std::memory_order_release);
        return device_->allocator() != nullptr;
    }

    void buffer_manager::deinitialize() noexcept
    {
        //~ teardown runs with the gpu idle so free everything inline the
        //~ slot_map only ever hands us the live ones
        buffers_.for_each([](slot& s) { release_slot_now(s); });
        buffers_.clear();

        device_   = nullptr;
        releaser_ = nullptr;
        arena_    = nullptr;
        need_rebuild_.store(true, std::memory_order_release);
    }

    //~ let go of everything a slot is holding unmap the cb mapping if any then
    //~ drop both refs the resource ref from ppvResource and the allocation
    void buffer_manager::release_slot_now(slot& s) noexcept
    {
        if (s.mapped && s.resource)
            s.resource->Unmap(0, nullptr);

        if (s.resource)   s.resource->Release();
        if (s.allocation) s.allocation->Release();

        s = slot{};
    }

    //~ build one buffer onto out no slot_map entry yet so a failure just leaves
    //~ an empty local for the caller to drop the gpu resource lands in COMMON
    //~ or GENERIC_READ for an upload heap cb
    bool buffer_manager::allocate_only(const buffer_create_info& info, slot& out)
    {
        if (!device_ || !device_->allocator() || info.size_bytes == 0)
            return false;

        slot& s  = out;
        s.size   = info.size_bytes;
        s.stride = info.stride;
        s.kind   = info.kind;

        const bool is_constant = (info.kind == buffer_kind::constant);
        const bool is_uav = (info.kind == buffer_kind::skinning_output)
                          || (info.kind == buffer_kind::default_uav);

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = is_constant ? D3D12_HEAP_TYPE_UPLOAD
                                          : D3D12_HEAP_TYPE_DEFAULT;

        const std::uint64_t alloc_size = statics::align_cb_size(info.size_bytes, is_constant);

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment          = 0u;
        desc.Width              = alloc_size;
        desc.Height             = 1u;
        desc.DepthOrArraySize   = 1u;
        desc.MipLevels          = 1u;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc         = { 1u, 0u };
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = is_uav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                                         : D3D12_RESOURCE_FLAG_NONE;

        const D3D12_RESOURCE_STATES initial_state = is_constant
            ? D3D12_RESOURCE_STATE_GENERIC_READ
            : D3D12_RESOURCE_STATE_COMMON;

        if (FAILED(device_->allocator()->CreateResource(
                &alloc_desc, &desc, initial_state, nullptr,
                &s.allocation, IID_PPV_ARGS(&s.resource))))
        {
            LOG_ERROR("CreateResource failed ({})", info.debug_name);
            s = slot{};       //~ nothing made leave out empty
            return false;
        }

        const std::wstring wname = info.debug_name
            ? statics::to_wide(info.debug_name)
            : std::wstring{};
        s.resource->SetName(wname.c_str());

        if (is_constant)
        {
            //~ upload heap cb so we map it once and keep the pointer the gpu
            //~ reads straight from here no copy the read range is empty since
            //~ the cpu only ever writes
            constexpr D3D12_RANGE no_read{ 0, 0 };
            if (FAILED(s.resource->Map(0, &no_read, &s.mapped)))
            {
                LOG_ERROR("CB Map failed ({})", info.debug_name);
                release_slot_now(s); //~ frees both refs and empties out
                return false;
            }
            if (info.initial_data)
                std::memcpy(s.mapped, info.initial_data, info.size_bytes);
        }

        return true;
    }

    buffer_handle buffer_manager::create(const buffer_create_info& info)
    {
        //~ build on a local nothing hits the slot_map until it all works out
        slot s{};
        if (!allocate_only(info, s)) return buffer_handle::invalid();

        //~ non constant with initial data rides the staged upload
        if (info.kind != buffer_kind::constant && info.initial_data)
        {
            if (!upload_static(s, info.initial_data, info.size_bytes))
            {
                release_slot_now(s); //~ upload died toss the whole buffer
                return buffer_handle::invalid();
            }
        }

        //~ all good drop it in the slot_map and let it mint the handle
        return buffers_.insert(std::move(s));
    }

    std::vector<buffer_handle>
        buffer_manager::create_batch(std::span<const buffer_create_info> infos)
    {
        std::vector<buffer_handle> out;
        out.reserve(infos.size());

        //~ build every buffer on a local slot first the vector is sized once up
        //~ front so the addresses hold still while we record the uploads nothing
        //~ enters the slot_map until the whole batch makes it all or nothing
        std::vector<slot> locals(infos.size());

        for (std::size_t i = 0; i < infos.size(); ++i)
        {
            if (!allocate_only(infos[i], locals[i]))
            {
                //~ one tanked unwind the lot the failed and untouched locals are
                //~ already empty so releasing them is a harmless noop
                for (auto& s : locals) release_slot_now(s);
                return {};
            }
        }

        //~ collect the staged uploads pointing straight at the stable locals
        //~ constant buffers already got their data from the persistent map
        std::vector<upload_record> uploads;
        uploads.reserve(infos.size());
        for (std::size_t i = 0; i < infos.size(); ++i)
        {
            const auto& info = infos[i];
            if (info.kind == buffer_kind::constant || !info.initial_data)
                continue;

            upload_record r{};
            r.dst  = &locals[i];
            r.data = info.initial_data;
            r.size = info.size_bytes;
            uploads.push_back(r);
        }

        //~ one staging one list one wait for the whole pile
        if (!uploads.empty() && !upload_batch(uploads))
        {
            LOG_ERROR("batch upload failed");
            for (auto& s : locals) release_slot_now(s); //~ all or nothing
            return {};
        }

        //~ all built and uploaded now droping each into the slot_map for its handle
        for (auto& s : locals)
            out.push_back(buffers_.insert(std::move(s)));

        LOG_INFO("batch created {} buffers in one flush", out.size());
        return out;
    }

    namespace
    {
        //~ where a buffer should sit once the upload is done index buffers go
        //~ to INDEX_BUFFER skinning source stays in COMMON so the compute pass
        //~ can promote it to an srv on its own and the plain draw path still
        //~ grabs it as a vb everything else lands in VERTEX_AND_CONSTANT_BUFFER
        D3D12_RESOURCE_STATES end_state_for(buffer_kind k) noexcept
        {
            switch (k)
            {
                case buffer_kind::index:
                    return D3D12_RESOURCE_STATE_INDEX_BUFFER;
                case buffer_kind::skinning_source:
                    return D3D12_RESOURCE_STATE_COMMON;
                default:
                    return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            }
        }
    } //~ anonymous namespace

    //~ push a whole pile of buffers up in one go the arena grabs staging from
    //~ its pool records every copy on one list and waits once a level worth of
    //~ buffers can ride a single batch way cheaper than one upload heap each
    bool buffer_manager::upload_batch(std::span<const upload_record> records) const
    {
        if (records.empty()) return true;
        if (!arena_)
        {
            LOG_ERROR("upload_batch called before upload arena wired");
            return false;
        }

        if (!arena_->begin_batch())
        {
            LOG_ERROR("arena begin_batch failed");
            return false;
        }

        for (const auto& r : records)
        {
            const auto state = end_state_for(r.dst->kind);
            if (!arena_->add_buffer_copy(r.dst->resource, r.data, r.size, state))
            {
                LOG_ERROR("arena add_buffer_copy failed");
                arena_->cancel_batch();
                return false;
            }
        }

        return arena_->submit_and_wait();
    }

    //~ just one buffer to upload still rides a one item arena batch the pool
    //~ recycles its staging across calls so we are not minting a fresh upload
    //~ heap blob every single time
    bool buffer_manager::upload_static(const slot& s, const void* data, const std::uint64_t size) const
    {
        if (!arena_)
        {
            LOG_ERROR("upload_static called before upload arena wired");
            return false;
        }

        if (!arena_->begin_batch())
        {
            LOG_ERROR("arena begin_batch failed");
            return false;
        }

        const auto state = end_state_for(s.kind);

        if (!arena_->add_buffer_copy(s.resource, data, size, state))
        {
            LOG_ERROR("arena add_buffer_copy failed");
            arena_->cancel_batch();
            return false;
        }

        return arena_->submit_and_wait();
    }

    void buffer_manager::destroy(const buffer_handle h)
    {
        slot* sp = buffers_.get(h);
        if (!sp) return; //~ stale or unknown handle already gone
        slot& s = *sp;

        //~ cpu side is done either way so close the mapping now even if the
        //~ gpu still reads the bytes the resource lives until we release it
        //~ clear mapped so the inline path below does not try to unmap twice
        //~ d3d12 keeps a map count and going negative is asking for trouble
        if (s.mapped && s.resource)
        {
            s.resource->Unmap(0, nullptr);
            s.mapped = nullptr;
        }

        //~ releaser wired so park the frees behind the fence a frame in flight
        //~ might still be touching this buffer not yanking it out yet
        if (releaser_)
        {
            releaser_->enqueue_allocation(s.allocation);

            //~ the resource ref goes through too attach takes our ref no extra
            //~ addref otherwise this leaks the resource object
            if (s.resource)
            {
                Microsoft::WRL::ComPtr<ID3D12Resource> res_com;
                res_com.Attach(s.resource);
                releaser_->enqueue_com(std::move(res_com));
            }
            s.allocation = nullptr;
            s.resource   = nullptr;
        }
        else //~ no releaser free right now and hope the gpu is done with it
        {
            release_slot_now(s); //~ mapping already closed so this just frees
        }
        buffers_.erase(h);
    }

    ID3D12Resource* buffer_manager::resource(const buffer_handle h) const
    {
        const slot* s = buffers_.get(h);
        return s ? s->resource : nullptr;
    }

    std::uint64_t buffer_manager::gpu_address(const buffer_handle h) const
    {
        auto* r = resource(h);
        return r ? r->GetGPUVirtualAddress() : 0;
    }

    std::uint64_t buffer_manager::size(const buffer_handle h) const
    {
        const slot* s = buffers_.get(h);
        return s ? s->size : 0u;
    }

    std::uint64_t buffer_manager::stride(const buffer_handle h) const
    {
        const slot* s = buffers_.get(h);
        return s ? s->stride : 0u;
    }

    void* buffer_manager::mapped_ptr(const buffer_handle h) const
    {
        const slot* s = buffers_.get(h);
        return s ? s->mapped : nullptr;
    }
} // namespace trishul::render::hardware
