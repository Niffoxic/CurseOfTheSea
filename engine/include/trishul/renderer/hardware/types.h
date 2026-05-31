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
#ifndef CURSEOFTHESEA_TYPES_H
#define CURSEOFTHESEA_TYPES_H

#include <cstdint>
#include <string>

namespace trishul::render::hardware
{
    enum class command_list_type: std::uint8_t
    {
        direct,
        compute,
        copy
    };

    inline std::string to_string(const command_list_type type) noexcept
    {
        switch(type)
        {
        case command_list_type::direct:  return "direct";
        case command_list_type::compute: return "compute";
        case command_list_type::copy:    return "copy";
        default: return "unknown";
        }
    }

    //~ resource state TODO: Use it for automation and extend as needed later
    enum class resource_state : std::uint32_t
    {
        common         = 0,
        render_target  = 1 << 0,
        present        = 1 << 1,
        depth_write    = 1 << 2,
        depth_read     = 1 << 3,
        shader_read    = 1 << 4,
        copy_source    = 1 << 5,
        copy_dest      = 1 << 6,
        unordered      = 1 << 7,
    };

    //~ basics fr now TODO: extend this later
    enum class format : std::uint8_t
    {
        unknown = 0,
        rgba8_unorm,
        rgba8_unorm_srgb,
        bgra8_unorm,
        bgra8_unorm_srgb,
        rgba16_float,
        d32_float,
        d24_unorm_s8_uint,
    };

    enum class display_mode : std::uint8_t
    {
        windowed             = 0,
        borderless           = 1,
        exclusive_fullscreen = 2,
    };
} // namespace trishul::render::hardware

#endif //CURSEOFTHESEA_TYPES_H
