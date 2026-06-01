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
#ifndef CURSEOFTHESEA_MESH_BAKED_STORAGE_H
#define CURSEOFTHESEA_MESH_BAKED_STORAGE_H

#include <cstdint>

namespace trishul::render::mesh
{
    //~ CMSH tag on the cooked mesh
    constexpr std::uint32_t k_mesh_magic        = 0x48534D43u;

    //~ bumps whenever they are changes on the layout
    constexpr std::uint32_t k_mesh_file_version = 1u;

    //~ is the cook still valid
    constexpr std::uint32_t k_mesh_bake_schema  = 1u;

    //~ fixed header the payload follows straight after stream blobs in format
    //~ flag order then indices then submeshes everything validated before any
    //~ allocation so a truncated or hostile file cannot make over allocate
    struct mesh_container_header
    {
        std::uint32_t magic;
        std::uint32_t file_version;
        std::uint32_t bake_schema;
        std::uint32_t format_flags;   //~ mesh_format bits present in the payload

        std::uint32_t vertex_count;
        std::uint32_t index_count;
        std::uint32_t submesh_count;
        std::uint32_t reserved;

        std::uint64_t source_hash;
        std::uint64_t payload_size;   //~ total bytes after this header

        float         bounds_min[3];  //~ local space mesh box
        float         bounds_max[3];
    };
    static_assert(sizeof(mesh_container_header) == 72u,
        "mesh_container_header must stay packed and stable across builds");

    //~ one packed submesh record in the payload mirrors the runtime submesh
    struct mesh_submesh_record
    {
        std::uint32_t index_offset;
        std::uint32_t index_count;
        std::uint32_t material_slot;
        float         bounds_min[3];
        float         bounds_max[3];
    };
    static_assert(sizeof(mesh_submesh_record) == 36u,
        "mesh_submesh_record must stay packed and stable across builds");
} // namespace trishul::render::mesh

#endif //CURSEOFTHESEA_MESH_BAKED_STORAGE_H
