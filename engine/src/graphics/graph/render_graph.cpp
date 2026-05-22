// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/graph/render_graph.h"

#include "engine/graphics/graph/declare_context.h"
#include "engine/graphics/passes/pass.h"
#include "engine/utils/profiler.h"

#include <spdlog/spdlog.h>

namespace cots::graphics::graph
{
    render_graph::~render_graph()
    {
        clear();
    }

    void render_graph::clear()
    {
        passes_.clear();
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

        spdlog::info("[graph] compiled {} pass(es), {} imported resource(s)",
                     passes_.size(), resources_.imports().size());
        return true;
    }

    void render_graph::execute(const execute_context& ec)
    {
        COTS_PROFILE_SCOPE("render_graph::execute");

        //~ resolve every import to its current per frame view
        resources_.refresh();

        const pass_context pc
        {
            .ctx         = ec.ctx,
            .snap        = ec.snap,
            .resources   = resources_,
            .width       = ec.width,
            .height      = ec.height,
            .frame_index = ec.frame_index,
        };

        for (const auto& node : passes_)
        {
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

        for (const auto&[p, reads, writes] : passes_)
        {
            const char* pn = p->name();

            for (const auto& r : reads)
            {
                if (!resources_.exists(r))
                {
                    spdlog::error("[graph] pass '{}' reads an unresolved handle "
                                  "(idx={}, gen={})", pn, r.index, r.generation);
                    return false;
                }
                if (!was_produced(r))
                {
                    spdlog::error("[graph] pass '{}' reads '{}' which nothing wrote",
                                  pn, resources_.debug_name(r));
                    return false;
                }
            }
            for (const auto& w : writes)
            {
                if (!resources_.exists(w))
                {
                    spdlog::error("[graph] pass '{}' writes an unresolved handle "
                                  "(idx={}, gen={})", pn, w.index, w.generation);
                    return false;
                }
                if (!was_produced(w)) produced.push_back(w);
            }
        }
        return true;
    }
} // namespace cots::graphics::graph
