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
#ifndef CURSEOFTHESEA_PRIMITIVES_H
#define CURSEOFTHESEA_PRIMITIVES_H

#include <cstdint>
#include "trishul/renderer/mesh/mesh_types.h"

namespace trishul::render::mesh::primitives
{
    [[nodiscard]] mesh_data cube      (float extent = 1.0f);
    [[nodiscard]] mesh_data plane     (float size = 1.0f, std::uint32_t subdivisions = 1u);
    [[nodiscard]] mesh_data uv_sphere (float radius = 0.5f, std::uint32_t rings = 16u, std::uint32_t sectors = 32u);
    [[nodiscard]] mesh_data ico_sphere(float radius = 0.5f, std::uint32_t subdivisions = 2u);
    [[nodiscard]] mesh_data cylinder  (float radius = 0.5f, float height = 1.0f, std::uint32_t segments = 32u);
    [[nodiscard]] mesh_data cone      (float radius = 0.5f, float height = 1.0f, std::uint32_t segments = 32u);
    [[nodiscard]] mesh_data capsule   (float radius = 0.5f, float body_height = 1.0f,
                                       std::uint32_t segments = 24u, std::uint32_t rings = 8u);
} // namespace trishul::render::mesh::primitives

#endif //CURSEOFTHESEA_PRIMITIVES_H
