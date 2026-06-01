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
#ifndef CURSEOFTHESEA_RESOURCE_HANDLE_H
#define CURSEOFTHESEA_RESOURCE_HANDLE_H

#include "trishul/core/slot_map.h"

namespace trishul::render::graph
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
} // namespace trishul::render::graph

#endif //CURSEOFTHESEA_RESOURCE_HANDLE_H
