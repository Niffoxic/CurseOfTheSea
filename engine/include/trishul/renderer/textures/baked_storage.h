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
#ifndef CURSEOFTHESEA_BAKED_STORAGE_H
#define CURSEOFTHESEA_BAKED_STORAGE_H

#include <cstdint>

namespace trishul::render::textures
{
    //~ magic tag sitting at the very top of a baked container so we can tell our
    //~ files apart from random bytes
    constexpr std::uint32_t k_container_magic = 0x58455443u;

    //~ can we even parse these bytes a mismatch means rebuild from scratch
    constexpr std::uint32_t k_container_file_version = 1u;

    //~ is the baked output still considered valid bump this when the bake recipe
    //~ changes so old caches get tossed
    constexpr std::uint32_t k_container_bake_schema = 1u;

    //~ the header the dds payload follows straight after it
    struct container_header
    {
        std::uint32_t magic;
        std::uint32_t file_version;
        std::uint32_t bake_schema;
        std::uint32_t intent;         //~ which texture_intent this was baked as
        std::uint64_t source_hash;    //~ hash of the source bytes
        std::uint64_t dds_size;       //~ payload size in bytes
    };

    static_assert(sizeof(container_header) == 32u,
        "container_header must stay packed and stable across builds");
} // namespace trishul::render::textures

#endif //CURSEOFTHESEA_BAKED_STORAGE_H
