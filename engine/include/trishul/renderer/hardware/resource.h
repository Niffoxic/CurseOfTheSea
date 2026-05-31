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

#include "trishul/core/slot_map.h"

//~ handles and the types the resource managers pass around the handles
//~ themselves come straight from core slot_map no point hand rolling the same
//~ index plus generation thing twice
namespace trishul::render::hardware
{
    struct texture_tag {};
    struct buffer_tag  {};

    using texture_handle = handle<texture_tag>;
    using buffer_handle  = handle<buffer_tag>;

    enum class texture_format : std::uint8_t
    {
        rgba8_unorm,
        rgba8_unorm_srgb,
    };

    enum class buffer_kind : std::uint8_t
    {
        vertex,
        index,
        constant,
        generic,    //~ structured or raw gpu default heap

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