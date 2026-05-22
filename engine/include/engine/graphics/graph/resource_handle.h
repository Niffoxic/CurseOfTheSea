// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GRAPH_RESOURCE_HANDLE_H
#define CURSEOFTHESEA_GRAPH_RESOURCE_HANDLE_H

#include <cstdint>

namespace cots::graphics::graph
{
    struct resource_handle
    {
        std::uint32_t index     { 0u };
        std::uint32_t generation{ 0u };

        [[nodiscard]] bool valid() const noexcept
        {
            return generation != 0u;
        }

        [[nodiscard]] static resource_handle invalid() noexcept
        {
            return { 0u, 0u };
        }

        bool operator==(const resource_handle& o) const noexcept
        {
            return index == o.index && generation == o.generation;
        }
        bool operator!=(const resource_handle& o) const noexcept
        {
            return !(*this == o);
        }
    };
} // namespace cots::graphics::graph

#endif //CURSEOFTHESEA_GRAPH_RESOURCE_HANDLE_H
