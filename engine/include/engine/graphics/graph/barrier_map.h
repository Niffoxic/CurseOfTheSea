// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GRAPH_BARRIER_MAP_H
#define CURSEOFTHESEA_GRAPH_BARRIER_MAP_H

#include <d3d12.h>
#include "engine/graphics/graph/resource_usage.h"

namespace cots::graphics::graph
{
    struct barrier_state
    {
        D3D12_BARRIER_SYNC   sync;
        D3D12_BARRIER_ACCESS access;
        D3D12_BARRIER_LAYOUT layout;
    };

    [[nodiscard]] barrier_state to_barrier_state(resource_usage u) noexcept;

    [[nodiscard]] inline bool operator==(const barrier_state& a, const barrier_state& b) noexcept
    {
        return a.sync == b.sync && a.access == b.access && a.layout == b.layout;
    }
    [[nodiscard]] inline bool operator!=(const barrier_state& a, const barrier_state& b) noexcept
    {
        return !(a == b);
    }
} // namespace cots::graphics::graph

#endif //CURSEOFTHESEA_GRAPH_BARRIER_MAP_H
