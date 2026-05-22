// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_MESH_BAKE_STORAGE_H
#define CURSEOFTHESEA_MESH_BAKE_STORAGE_H

#include <cstdint>

namespace cots::graphics::meshes
{
    constexpr std::uint32_t k_mesh_container_magic = 0x48534D43u;

    //~ can these bytes be parsed
    constexpr std::uint32_t k_mesh_container_file_version = 1u;

    //~ is this output still valid
    constexpr std::uint32_t k_mesh_container_bake_schema = 1u;

    //~ header for a baked mesh
    //~ records and payloads follow
    struct mesh_container_header
    {
        std::uint32_t magic;
        std::uint32_t file_version;
        std::uint32_t bake_schema;
        std::uint32_t stream_count;
        std::uint32_t vertex_count;
        std::uint32_t index_count;
        std::uint32_t index_16bit;     //~ bool
        std::uint32_t index_bytes;     //~ payload size
        std::uint64_t source_hash;
    };

    static_assert(sizeof(mesh_container_header) == 40u,
        "mesh_container_header must be stable across builds");

    //~ per stream descriptor
    struct mesh_stream_record
    {
        std::uint32_t semantic_len;    //~ bytes
        std::uint32_t stride;
        std::uint32_t bytes;           //~ payload size for this stream
        std::uint32_t reserved;
    };

    static_assert(sizeof(mesh_stream_record) == 16u,
        "mesh_stream_record must be stable across builds");
} // namespace cots::graphics::meshes

#endif //CURSEOFTHESEA_MESH_BAKE_STORAGE_H
