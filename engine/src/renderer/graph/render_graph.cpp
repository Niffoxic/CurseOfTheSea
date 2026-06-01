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
#include "trishul/renderer/graph/render_graph.h"
#include "trishul/renderer/hardware/command_context.h"
#include "trishul/utils/logger.h"

#include <d3d12.h>

namespace trishul::render::graph
{
    void render_graph::setup(const setup_context& sc)
    {
        for (auto& e : entries_)
        {
            if (e.instance && !e.instance->setup(sc))
                LOG_ERROR("pass '{}' setup failed", e.instance->name());
        }
    }

    void render_graph::compile()
    {
        //~ ask every pass what it reads and writes once and cache it re running
        //~ declare each frame would just churn allocs
        for (auto& e : entries_)
        {
            e.reads.clear();
            e.writes.clear();
            if (!e.instance) continue;

            declare_context dc(e.reads, e.writes);
            e.instance->declare(dc);
        }
        dirty_ = false;
    }

    void render_graph::transition(
        hardware::command_context&          ctx,
        const std::vector<resource_access>& reads,
        const std::vector<resource_access>& writes)
    {
        //~ merge this passes accesses per resource writes win over reads so a
        //~ resource touched both ways lands in its write state not a read one
        std::unordered_map<ID3D12Resource*, resource_usage> wanted;
        wanted.reserve(reads.size() + writes.size());

        auto collect = [&](const std::vector<resource_access>& list)
        {
            for (const auto& a : list)
            {
                const auto& view = registry_.view(a.handle);
                if (!view.resource) continue; //~ provider had nothing this frame
                wanted[view.resource] = a.usage;
            }
        };
        collect(reads);
        collect(writes); //~ second so writes overwrite reads

        if (wanted.empty()) return;

        std::vector<D3D12_TEXTURE_BARRIER> barriers;
        barriers.reserve(wanted.size());

        for (const auto& [res, usage] : wanted)
        {
            const barrier_state next = to_barrier_state(usage);

            //~ unseen resource is assumed common thats the directx default
            barrier_state cur{ D3D12_BARRIER_SYNC_NONE,
                               D3D12_BARRIER_ACCESS_NO_ACCESS,
                               D3D12_BARRIER_LAYOUT_COMMON };
            if (const auto it = states_.find(res); it != states_.end())
                cur = it->second;

            if (cur == next) continue; //~ already where it needs to be skip it

            D3D12_TEXTURE_BARRIER b{};
            b.SyncBefore   = cur.sync;
            b.SyncAfter    = next.sync;
            b.AccessBefore = cur.access;
            b.AccessAfter  = next.access;
            b.LayoutBefore = cur.layout;
            b.LayoutAfter  = next.layout;
            b.pResource    = res;
            //~ first mip means every subresource at once
            b.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE{
                0xffffffffu, 0, 0,
                0, 0, 0 };
            b.Flags        = D3D12_TEXTURE_BARRIER_FLAG_NONE;

            barriers.push_back(b);
            states_[res] = next;
        }

        if (barriers.empty()) return;

        D3D12_BARRIER_GROUP group{};
        group.Type             = D3D12_BARRIER_TYPE_TEXTURE;
        group.NumBarriers      = static_cast<UINT>(barriers.size());
        group.pTextureBarriers = barriers.data();

        ctx.list()->Barrier(1, &group);
    }

    void render_graph::execute(const frame_desc& fd)
    {
        if (dirty_) compile();

        //~ refresh the per frame views
        registry_.refresh();

        for (auto& e : entries_)
        {
            if (!e.instance) continue;

            //~ throttled passes only fire when their pacer says so
            float dt = fd.delta_seconds;
            if (!e.pacer.should_run(e.instance->update_hz(), fd.delta_seconds, dt))
                continue;

            transition(fd.ctx, e.reads, e.writes);

            const pass_context pc
            {
                fd.ctx,
                fd.snap,
                registry_,
                fd.width,
                fd.height,
                fd.frame_index,
                dt,
                fd.elapsed_seconds
            };
            e.instance->execute(pc);
        }
    }

    void render_graph::invalidate() noexcept
    {
        //~ every tracked pointer just went stale wipe it and recompile
        states_.clear();
        dirty_ = true;
    }
} // namespace trishul::render::graph
