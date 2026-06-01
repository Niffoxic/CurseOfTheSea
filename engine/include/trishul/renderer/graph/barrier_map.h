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
#ifndef CURSEOFTHESEA_BARRIER_MAP_H
#define CURSEOFTHESEA_BARRIER_MAP_H

#include <d3d12.h>
#include "resource_usage.h"

namespace trishul::render::graph
{
    struct barrier_state
    {
        D3D12_BARRIER_SYNC   sync;
        D3D12_BARRIER_ACCESS access;
        D3D12_BARRIER_LAYOUT layout;
    };

    [[nodiscard]]
    inline barrier_state to_barrier_state(const resource_usage u) noexcept
    {
        switch (u)
        {
        case resource_usage::common:
            return { D3D12_BARRIER_SYNC_NONE,
                     D3D12_BARRIER_ACCESS_NO_ACCESS,
                     D3D12_BARRIER_LAYOUT_COMMON };

        case resource_usage::render_target:
            return { D3D12_BARRIER_SYNC_RENDER_TARGET,
                     D3D12_BARRIER_ACCESS_RENDER_TARGET,
                     D3D12_BARRIER_LAYOUT_RENDER_TARGET };

        case resource_usage::depth_write:
            return { D3D12_BARRIER_SYNC_DEPTH_STENCIL,
                     D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
                     D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE };

        case resource_usage::depth_read:
            return { D3D12_BARRIER_SYNC_DEPTH_STENCIL,
                     D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ,
                     D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ };

        case resource_usage::shader_read: //~ generic sr sampling from any stage
            return { D3D12_BARRIER_SYNC_ALL_SHADING,
                     D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
                     D3D12_BARRIER_LAYOUT_SHADER_RESOURCE };

        case resource_usage::pixel_shader_resource:
            //~ sampled in pixel stage
            return { D3D12_BARRIER_SYNC_PIXEL_SHADING,
                     D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
                     D3D12_BARRIER_LAYOUT_SHADER_RESOURCE };

        case resource_usage::copy_source:
            return { D3D12_BARRIER_SYNC_COPY,
                     D3D12_BARRIER_ACCESS_COPY_SOURCE,
                     D3D12_BARRIER_LAYOUT_COPY_SOURCE };

        case resource_usage::copy_dest:
            return { D3D12_BARRIER_SYNC_COPY,
                     D3D12_BARRIER_ACCESS_COPY_DEST,
                     D3D12_BARRIER_LAYOUT_COPY_DEST };

        case resource_usage::unordered_access:
            return { D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                     D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
                     D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS };

        case resource_usage::present:
            return { D3D12_BARRIER_SYNC_NONE,
                     D3D12_BARRIER_ACCESS_NO_ACCESS,
                     D3D12_BARRIER_LAYOUT_PRESENT };
        }

        return { D3D12_BARRIER_SYNC_NONE,
                 D3D12_BARRIER_ACCESS_NO_ACCESS,
                 D3D12_BARRIER_LAYOUT_COMMON };
    }

    [[nodiscard]] inline bool operator==(const barrier_state& a, const barrier_state& b) noexcept
    {
        return a.sync == b.sync && a.access == b.access && a.layout == b.layout;
    }
    [[nodiscard]] inline bool operator!=(const barrier_state& a, const barrier_state& b) noexcept
    {
        return !(a == b);
    }
} // namespace trishul::render::graph

#endif //CURSEOFTHESEA_BARRIER_MAP_H
