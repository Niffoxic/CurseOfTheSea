// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/graph/render_graph.h"

#include "engine/graphics/passes/pass.h"
#include "engine/utils/profiler.h"

#include <spdlog/spdlog.h>
#include <d3d12.h>

namespace cots::graphics::graph
{
    namespace
    {
        //~ all sub resources pattern gonna beed for the enhanced barrier convention
        constexpr D3D12_BARRIER_SUBRESOURCE_RANGE k_all_subresources
        {
            .IndexOrFirstMipLevel = 0,
            .NumMipLevels         = 0xffffffffu,
            .FirstArraySlice      = 0,
            .NumArraySlices       = 0xffffffffu,
            .FirstPlane           = 0,
            .NumPlanes            = 0xffffffffu,
        };
    } // namespace anonymous

    render_graph::~render_graph()
    {
        clear();
    }

    void render_graph::clear()
    {
        passes_   .clear();
        states_   .clear();
        resources_.clear();
    }

    void render_graph::add_pass(std::unique_ptr<pass> p)
    {
        if (!p)
            return;

        pass_node node{};
        node.p = std::move(p);
        passes_.push_back(std::move(node));
    }

    bool render_graph::compile(const setup_context& sc)
    {
        COTS_PROFILE_SCOPE("render_graph::compile");

        //~ pass setup
        for (const auto& node : passes_)
        {
            if (!node.p->setup(sc))
            {
                spdlog::error("[graph] pass '{}' setup failed", node.p->name());
                return false;
            }
        }

        //~ declarations
        for (auto&[p, reads, writes] : passes_)
        {
            reads .clear();
            writes.clear();
            declare_context dc{ reads, writes };
            p->declare(dc);
        }

        if (!validate())
            return false;

        states_.assign(resources_.size(), tracked_resource{});
        spdlog::info("[graph] compiled {} pass(es), {} imported resource(s)",
                     passes_.size(), resources_.imports().size());
        return true;
    }

    void render_graph::execute(const execute_context& ec)
    {
        COTS_PROFILE_SCOPE("render_graph::execute");

        //~ resolve every import to its current per frame view
        resources_.refresh  ();
        sync_resource_states();

        const pass_context pc
        {
            .ctx         = ec.ctx,
            .snap        = ec.snap,
            .resources   = resources_,
            .width       = ec.width,
            .height      = ec.height,
            .frame_index = ec.frame_index,
        };

        for (auto& node : passes_)
        {
            emit_barriers_for_pass(node, ec.ctx);
            node.p->execute(pc);
        }
    }

    std::uint32_t render_graph::pass_count() const noexcept
    {
        return static_cast<std::uint32_t>(passes_.size());
    }

    bool render_graph::validate() const
    {
        //~ every imported resource counts as produced from frame start
        std::vector<resource_handle> produced;
        produced.reserve(resources_.size());
        for (const auto& h : resources_.imports())
        {
            produced.push_back(h);
        }

        auto was_produced = [&](const resource_handle h) -> bool
        {
            for (const auto& p : produced)
            {
                if (p == h) return true;
            }
            return false;
        };

        for (const auto& node: passes_)
        {
            const char* pn = node.p->name();

            for (const auto& a : node.reads)
            {
                if (!resources_.exists(a.handle))
                {
                    spdlog::error("[graph] pass '{}' reads an unresolved handle "
                                "(idx={}, gen={}) as {}",
                              pn, a.handle.index, a.handle.generation,
                                to_string(a.usage)
                    );
                    return false;
                }
                if (!was_produced(a.handle))
                {
                    spdlog::error("[graph] pass '{}' reads '{}' as {} but nothing wrote it",
                                  pn, resources_.debug_name(a.handle),
                                  to_string(a.usage));
                    return false;
                }
            }
            for (const auto& w : node.writes)
            {
                if (!resources_.exists(w.handle))
                {
                    spdlog::error("[graph] pass '{}' writes an unresolved handle "
                                "(idx={}, gen={}) as {}",
                                pn, w.handle.index, w.handle.generation,
                                to_string(w.usage)
                    );
                    return false;
                }
                if (!was_produced(w.handle))
                    produced.push_back(w.handle);
            }
        }
        return true;
    }

     void render_graph::sync_resource_states()
    {
        const std::uint32_t n = resources_.size();

        if (states_.size() != n) states_.resize(n);

        for (const auto h : resources_.imports())
        {
            if (h.index >= states_.size()) continue;
            auto& tracked = states_[h.index];
            const auto& v = resources_.view(h);

            if (!tracked.initialized || tracked.last_seen != v.resource)
            {
                tracked.current     = to_barrier_state(resources_.initial_usage(h));
                tracked.last_seen   = v.resource;
                tracked.initialized = true;
            }
        }
    }

    void render_graph::emit_barriers_for_pass(const pass_node& node,
                                              hardware::command_context& ctx)
    {
        std::vector<D3D12_TEXTURE_BARRIER> texture_barriers;
        texture_barriers.reserve(node.reads.size() + node.writes.size());

        auto process = [&](const resource_access& access)
        {
            if (!resources_.exists(access.handle)) return;

            const auto& v = resources_.view(access.handle);
            if (!v.resource) return;
            if (access.handle.index >= states_.size()) return;

            auto&            tracked  = states_[access.handle.index];
            const auto       required = to_barrier_state(access.usage);

            if (tracked.current == required) return;

            D3D12_TEXTURE_BARRIER b{};
            b.SyncBefore   = tracked.current.sync;
            b.SyncAfter    = required.sync;
            b.AccessBefore = tracked.current.access;
            b.AccessAfter  = required.access;
            b.LayoutBefore = tracked.current.layout;
            b.LayoutAfter  = required.layout;
            b.pResource    = v.resource;
            b.Subresources = k_all_subresources;
            b.Flags        = D3D12_TEXTURE_BARRIER_FLAG_NONE;

            texture_barriers.push_back(b);
            tracked.current = required;
        };

        for (const auto& a : node.reads)  process(a);
        for (const auto& a : node.writes) process(a);

        if (texture_barriers.empty()) return;

        const D3D12_BARRIER_GROUP group
        {
            .Type             = D3D12_BARRIER_TYPE_TEXTURE,
            .NumBarriers      = static_cast<UINT32>(texture_barriers.size()),
            .pTextureBarriers = texture_barriers.data(),
        };

        auto* list = ctx.list();
        if (list)
            list->Barrier(1, &group);
    }

} // namespace cots::graphics::graph
