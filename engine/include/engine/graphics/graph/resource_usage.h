// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GRAPH_RESOURCE_USAGE_H
#define CURSEOFTHESEA_GRAPH_RESOURCE_USAGE_H

#include <cstdint>

namespace cots::graphics::graph
{
    //~ how a pass intends to access a resource for one frame
    enum class resource_usage : std::uint8_t
    {
        common = 0,
        render_target,
        depth_write,
        depth_read,
        shader_read,
        copy_source,
        copy_dest,
        unordered_access,
        present,
    };

    [[nodiscard]] inline const char* to_string(const resource_usage u) noexcept
    {
        switch (u)
        {
        case resource_usage::common:           return "common";
        case resource_usage::render_target:    return "render_target";
        case resource_usage::depth_write:      return "depth_write";
        case resource_usage::depth_read:       return "depth_read";
        case resource_usage::shader_read:      return "shader_read";
        case resource_usage::copy_source:      return "copy_source";
        case resource_usage::copy_dest:        return "copy_dest";
        case resource_usage::unordered_access: return "unordered_access";
        case resource_usage::present:          return "present";
        }
        return "?";
    }
} // namespace cots::graphics::graph

#endif //CURSEOFTHESEA_GRAPH_RESOURCE_USAGE_H
