// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_BAKED_STORAGE_H
#define CURSEOFTHESEA_BAKED_STORAGE_H

#include <cstdint>

namespace cots::graphics::textures
{
    //~ magic for the baked container
    constexpr std::uint32_t k_container_magic = 0x58455443u;

    //~ can these bytes be parsed
    constexpr std::uint32_t k_container_file_version = 1u;

    //~ is this output still valid
    constexpr std::uint32_t k_container_bake_schema = 1u;

    //~ header for a baked texture
    //~ dds payload follows
    struct container_header
    {
        std::uint32_t magic;
        std::uint32_t file_version;
        std::uint32_t bake_schema;
        std::uint32_t intent;         //~ texture intent
        std::uint64_t source_hash;    //~ source bytes hash
        std::uint64_t dds_size;       //~ payload size
    };

    static_assert(sizeof(container_header) == 32u,
        "container_header must be packed and stable across builds");
} // namespace cots::graphics::textures

#endif //CURSEOFTHESEA_BAKED_STORAGE_H
