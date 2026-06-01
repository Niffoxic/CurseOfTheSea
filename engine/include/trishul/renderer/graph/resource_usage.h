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
#ifndef CURSEOFTHESEA_RESOURCE_USAGE_H
#define CURSEOFTHESEA_RESOURCE_USAGE_H

#include <cstdint>

namespace trishul::render::graph
{
    //~ how a pass is supposed to be accessing a resource for one frame
    enum class resource_usage : std::uint8_t
    {
        common = 0,
        render_target,
        depth_write,
        depth_read,
        shader_read,
        pixel_shader_resource,
        copy_source,
        copy_dest,
        unordered_access,
        present,
    };

    [[nodiscard]] inline const char* to_string(const resource_usage u) noexcept
    {
        switch (u)
        {
        case resource_usage::common:                return "common";
        case resource_usage::render_target:         return "render_target";
        case resource_usage::depth_write:           return "depth_write";
        case resource_usage::depth_read:            return "depth_read";
        case resource_usage::shader_read:           return "shader_read";
        case resource_usage::pixel_shader_resource: return "pixel_shader_resource";
        case resource_usage::copy_source:           return "copy_source";
        case resource_usage::copy_dest:             return "copy_dest";
        case resource_usage::unordered_access:      return "unordered_access";
        case resource_usage::present:               return "present";
        }
        return "?";
    }
} // namespace trishul::render::graph

#endif //CURSEOFTHESEA_RESOURCE_USAGE_H
