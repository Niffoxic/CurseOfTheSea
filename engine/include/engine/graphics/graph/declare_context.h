// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GRAPH_DECLARE_CONTEXT_H
#define CURSEOFTHESEA_GRAPH_DECLARE_CONTEXT_H

#include <vector>
#include "engine/graphics/graph/resource_handle.h"
#include "engine/graphics/graph/resource_usage.h"

namespace cots::graphics::graph
{
    struct resource_access
    {
        resource_handle handle;
        resource_usage  usage;
    };

    class declare_context final
    {
    public:
        declare_context(std::vector<resource_access>& reads,
                           std::vector<resource_access>& writes) noexcept
        : reads_(reads), writes_(writes)
        {}

        declare_context           (const declare_context&) = delete;
        declare_context& operator=(const declare_context&) = delete;

        void read (const resource_handle h, const resource_usage u)
        {
            reads_ .push_back({ h, u });
        }
        void write(const resource_handle h, const resource_usage u)
        {
            writes_.push_back({ h, u });
        }

    private:
        std::vector<resource_access>& reads_;
        std::vector<resource_access>& writes_;
    };
} // namespace cots::graphics::graph

#endif //CURSEOFTHESEA_GRAPH_DECLARE_CONTEXT_H
