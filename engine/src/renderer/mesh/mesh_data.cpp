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
#include "trishul/renderer/mesh/mesh_types.h"
#include <cmath>

namespace trishul::render::mesh
{
    void mesh_data::recompute_bounds() noexcept
    {
        bounds = aabb{};
        sphere = bounding_sphere{};
        if (positions.empty()) return;

        //~ whole mesh box first
        for (const auto& p : positions) bounds.grow(p);

        //~ sphere centred on the box max radius to any point so it always wraps
        const DirectX::XMFLOAT3 c = bounds.center();
        float r2 = 0.f;
        for (const auto& p : positions)
        {
            const float dx = p.x - c.x;
            const float dy = p.y - c.y;
            const float dz = p.z - c.z;
            const float d2 = dx * dx + dy * dy + dz * dz;
            if (d2 > r2) r2 = d2;
        }
        sphere.center = c;
        sphere.radius = std::sqrt(r2);

        //~ each submesh gets its own box from the verts its slice touches handy
        //~ for per draw culling later
        const std::uint32_t vcount = vertex_count();
        for (auto& sm : submeshes)
        {
            sm.bounds = aabb{};
            const std::uint32_t end = sm.index_offset + sm.index_count;
            for (std::uint32_t i = sm.index_offset; i < end && i < indices.size(); ++i)
            {
                const std::uint32_t vi = indices[i];
                if (vi < vcount) sm.bounds.grow(positions[vi]);
            }
        }
    }

    bool mesh_data::valid() const noexcept
    {
        if (positions.empty() || indices.empty()) return false;
        if (indices.size() % 3u != 0u)            return false; //~ triangle list

        const std::uint32_t vcount = vertex_count();
        for (const std::uint32_t i : indices)
            if (i >= vcount) return false; //~ index points past the verts

        //~ every submesh slice must sit inside the index buffer
        for (const auto& sm : submeshes)
        {
            const std::uint64_t end =
                static_cast<std::uint64_t>(sm.index_offset) + sm.index_count;
            if (end > indices.size()) return false;
        }
        return true;
    }
} // namespace trishul::render::mesh
