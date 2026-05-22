// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GRAPH_RENDER_GRAPH_H
#define CURSEOFTHESEA_GRAPH_RENDER_GRAPH_H

#include <cstdint>
#include <memory>
#include <vector>

#include "engine/graphics/graph/barrier_map.h"
#include "engine/graphics/graph/declare_context.h"
#include "engine/graphics/graph/resource_registry.h"

struct ID3D12Resource2;

namespace cots::graphics
{
    namespace hardware { class command_context; }
    struct scene_snapshot;
    struct setup_context;
    class  pass;
} // namespace cots::graphics

namespace cots::graphics::graph
{
    //~ everything the graph needs to record one frame
    struct execute_context
    {
        hardware::command_context& ctx;
        const scene_snapshot&      snap;
        std::uint32_t              width;
        std::uint32_t              height;
        std::uint32_t              frame_index;
    };

    //~ just an orchestration layer
    class render_graph final
    {
    public:
         render_graph() = default;
        ~render_graph();

        render_graph           (const render_graph&) = delete;
        render_graph& operator=(const render_graph&) = delete;
        render_graph           (render_graph&&)      = delete;
        render_graph& operator=(render_graph&&)      = delete;

        //~ wipe passes and resources
        void clear();

        //~ append a pass
        void add_pass(std::unique_ptr<pass> p);

        [[nodiscard]] bool compile(const setup_context& sc);

        //~ refresh imports derive and emit barriers per pass
        void execute(const execute_context& ec);

        resource_registry&       resources() noexcept       { return resources_; }
        const resource_registry& resources() const noexcept { return resources_; }

        [[nodiscard]] std::uint32_t pass_count() const noexcept;

    private:
        struct pass_node
        {
            std::unique_ptr<pass>        p;
            std::vector<resource_access> reads;
            std::vector<resource_access> writes;
        };

        struct tracked_resource
        {
            barrier_state    current     { D3D12_BARRIER_SYNC_NONE,
                                           D3D12_BARRIER_ACCESS_NO_ACCESS,
                                           D3D12_BARRIER_LAYOUT_COMMON };
            ID3D12Resource2* last_seen   { nullptr };
            bool             initialized { false };
        };

        //~ internals
        [[nodiscard]] bool validate() const;

        void sync_resource_states  ();
        void emit_barriers_for_pass(const pass_node& node,
                                    hardware::command_context& ctx);

        private:
        std::vector<pass_node>        passes_;
        std::vector<tracked_resource> states_;
        resource_registry             resources_;
    };
} // namespace cots::graphics::graph

#endif //CURSEOFTHESEA_GRAPH_RENDER_GRAPH_H
