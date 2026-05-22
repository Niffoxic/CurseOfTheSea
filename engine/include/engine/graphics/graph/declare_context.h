// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GRAPH_DECLARE_CONTEXT_H
#define CURSEOFTHESEA_GRAPH_DECLARE_CONTEXT_H

#include <vector>
#include "engine/graphics/graph/resource_handle.h"

namespace cots::graphics::graph
{
    class declare_context final
    {
    public:
        declare_context(std::vector<resource_handle>& reads,
                        std::vector<resource_handle>& writes) noexcept
            :
        reads_(reads), writes_(writes)
        {}

        declare_context           (const declare_context&) = delete;
        declare_context& operator=(const declare_context&) = delete;

        void read (resource_handle h) { reads_ .push_back(h); }
        void write(resource_handle h) { writes_.push_back(h); }

    private:
        std::vector<resource_handle>& reads_;
        std::vector<resource_handle>& writes_;
    };
} // namespace cots::graphics::graph

#endif //CURSEOFTHESEA_GRAPH_DECLARE_CONTEXT_H
