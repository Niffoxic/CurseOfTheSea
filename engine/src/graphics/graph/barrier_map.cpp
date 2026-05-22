// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/graph/barrier_map.h"

namespace cots::graphics::graph
{
    barrier_state to_barrier_state(const resource_usage u) noexcept
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

        case resource_usage::shader_read:
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
} // namespace cots::graphics::graph
