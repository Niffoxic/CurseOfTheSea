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
#include "trishul/renderer/mesh/mesh_registry.h"

namespace trishul::render::mesh
{
    mesh_handle mesh_registry::add(mesh_data data)
    {
        return meshes_.insert(std::move(data));
    }

    mesh_data* mesh_registry::get(const mesh_handle h) noexcept
    {
        return meshes_.get(h);
    }

    const mesh_data* mesh_registry::get(const mesh_handle h) const noexcept
    {
        return meshes_.get(h);
    }

    bool mesh_registry::remove(const mesh_handle h) noexcept
    {
        return meshes_.erase(h);
    }

    void mesh_registry::clear() noexcept
    {
        meshes_.clear();
    }

    std::uint32_t mesh_registry::count() const noexcept
    {
        return meshes_.size();
    }

    mesh_stats mesh_registry::stats() const noexcept
    {
        mesh_stats s{};
        s.loaded_meshes = meshes_.size();
        meshes_.for_each([&](const mesh_data& m)
        {
            s.total_vertices += m.vertex_count();
            s.total_indices  += m.index_count();
            s.cpu_bytes      += m.cpu_bytes();
        });
        return s;
    }
} // namespace trishul::render::mesh
