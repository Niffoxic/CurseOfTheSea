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
#ifndef CURSEOFTHESEA_RESOURCE_H
#define CURSEOFTHESEA_RESOURCE_H

#include <cstdint>

//~ has handles and info related to resources
namespace trishul::render::hardware
{
    struct texture_handle
    {
        std::uint32_t index     { 0u };
        std::uint32_t generation{ 0u };

        [[nodiscard]] bool valid() const noexcept
        {
            return generation != 0u;
        }

        [[nodiscard]] static texture_handle invalid() noexcept
        {
            return { 0u, 0u };
        }

        bool operator==(const texture_handle& o) const noexcept
        {
            return index == o.index && generation == o.generation;
        }
    };

    enum class texture_format : std::uint8_t
    {
        rgba8_unorm,
        rgba8_unorm_srgb,
    };

    struct buffer_handle
    {
        std::uint32_t index     { 0u };
        std::uint32_t generation{ 0u };

        [[nodiscard]] bool valid() const noexcept
        {
            return generation != 0u;
        }

        [[nodiscard]] static buffer_handle invalid() noexcept
        {
            return { 0u, 0u };
        }

        bool operator==(const buffer_handle& o) const noexcept
        {
            return index == o.index && generation == o.generation;
        }
    };

    enum class buffer_kind : std::uint8_t
    {
        vertex,
        index,
        constant,
        generic,    //~ structured or raw gpu dfault heap

        //~ source vertex data for the compute skinning pass it reads as a root
        //  srv the bind pose position normal joints weights streams
        skinning_source,

        //~ default heap buffer with allow unordered access
        skinning_output,

        //~ general purpose uav capable
        default_uav,
    };
} // namespace trishul::render::hardware

#endif
