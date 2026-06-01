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
#ifndef CURSEOFTHESEA_MESH_REGISTRY_H
#define CURSEOFTHESEA_MESH_REGISTRY_H

#include <cstdint>
#include "trishul/renderer/mesh/mesh_types.h"

namespace trishul::render::mesh
{
    //~ what the debug overlay shows for debugging from the gameplay side
    struct mesh_stats
    {
        std::uint32_t loaded_meshes { 0u };
        std::uint64_t total_vertices{ 0u };
        std::uint64_t total_indices { 0u };
        std::uint64_t cpu_bytes     { 0u };
    };

    //~ owns the loaded cpu meshes behind stable handles
    class mesh_registry final
    {
    public:
         mesh_registry() = default;
        ~mesh_registry() = default;

        mesh_registry(const mesh_registry&) = delete;
        mesh_registry& operator=(const mesh_registry&) = delete;

        //~ takes ownership of the mesh and hands back its stable handle
        [[nodiscard]] mesh_handle add(mesh_data data);

        [[nodiscard]] mesh_data*       get(mesh_handle h)       noexcept;
        [[nodiscard]] const mesh_data* get(mesh_handle h) const noexcept;

        bool remove(mesh_handle h) noexcept;
        void clear()               noexcept;

        [[nodiscard]] std::uint32_t count() const noexcept;
        [[nodiscard]] mesh_stats    stats() const noexcept;

    private:
        slot_map<mesh_data, mesh_tag> meshes_;
    };
} // namespace trishul::render::mesh

#endif //CURSEOFTHESEA_MESH_REGISTRY_H
