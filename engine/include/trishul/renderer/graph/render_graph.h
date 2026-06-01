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
#ifndef CURSEOFTHESEA_RENDER_GRAPH_H
#define CURSEOFTHESEA_RENDER_GRAPH_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "trishul/renderer/pass/pass.h"
#include "trishul/renderer/pass/pass_pacer.h"
#include "trishul/renderer/graph/barrier_map.h"
#include "trishul/renderer/graph/declare_context.h"
#include "trishul/renderer/graph/resource_registry.h"

struct ID3D12Resource;

namespace trishul::render::graph
{
    //~ the compiler bit takes whatever passes get added imports the resources
    //  they touch then every frame walks the list dropping in the right barriers
    //  before each pass so I can just focus on adding passes
    class render_graph final
    {
    public:
         render_graph() = default;
        ~render_graph() = default;

        render_graph           (const render_graph&) = delete;
        render_graph& operator=(const render_graph&) = delete;
        render_graph           (render_graph&&)      = delete;
        render_graph& operator=(render_graph&&)      = delete;

        //~ build a pass in place and own it returns the raw pointer so the caller
        //~ can hold a typed handle for later tweaking
        template<typename T, typename... Args>
        T* add_pass(Args&&... args)
        {
            auto  owned = std::make_unique<T>(std::forward<Args>(args)...);
            T*    raw   = owned.get();
            entries_.push_back(entry{ std::move(owned), {}, {}, {} });
            dirty_ = true;
            return raw;
        }

        //~ register an external resource
        resource_handle import(const char*          debug_name,
                               const resource_usage initial_usage,
                               resource_provider    provider,
                               const bool           preserve_contents = false)
        {
            dirty_ = true;
            return registry_.import(debug_name, initial_usage,
                                    std::move(provider), preserve_contents);
        }

        //~ one time gpu resource creation for every pass call again after a
        //~ device rebuild so passes can recreate their psos and buffers
        void setup(const setup_context& sc);

        //~ everything one frame of recording needs the graph fills the rest
        struct frame_desc
        {
            hardware::command_context& ctx;
            const scene_snapshot&      snap;
            std::uint32_t              width;
            std::uint32_t              height;
            std::uint32_t              frame_index;
            float                      delta_seconds;
            double                     elapsed_seconds;
        };

        //~ record the whole graph into the frames command list
        void execute(const frame_desc& fd);

        //~ drop tracked gpu state and force a recompile
        void invalidate() noexcept;

        [[nodiscard]] resource_registry&       resources()       noexcept { return registry_; }
        [[nodiscard]] const resource_registry& resources() const noexcept { return registry_; }
        [[nodiscard]] std::size_t              pass_count() const noexcept { return entries_.size(); }

    private:
        struct entry
        {
            std::unique_ptr<pass>        instance;
            std::vector<resource_access> reads;
            std::vector<resource_access> writes;
            pass_pacer                   pacer;
        };

        //~ run every passes declare into its cached read write lists
        void compile();

        //~ emit the barriers one pass needs before it records writes win over
        //~ reads when a pass touches the same resource both ways
        void transition(hardware::command_context&          ctx,
                        const std::vector<resource_access>& reads,
                        const std::vector<resource_access>& writes);

        resource_registry                                  registry_;
        std::vector<entry>                                 entries_;
        std::unordered_map<ID3D12Resource*, barrier_state> states_;
        bool                                               dirty_{ true };
    };
} // namespace trishul::render::graph

#endif //CURSEOFTHESEA_RENDER_GRAPH_H
