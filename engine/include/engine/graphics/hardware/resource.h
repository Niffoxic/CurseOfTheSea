// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_RESOURCE_H
#define CURSEOFTHESEA_RESOURCE_H

#include <cstdint>

namespace cots::graphics::hardware
{
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
        generic,    //~ structured or raw GPU-default heap
    };
} // namespace cots::graphics::hardware

#endif
