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
#ifndef CURSEOFTHESEA_TERRAIN_H
#define CURSEOFTHESEA_TERRAIN_H

#include <cstdint>
#include "trishul/renderer/mesh/mesh_types.h"

namespace trishul::render::mesh
{
    //~ fully editable terrain
    struct terrain_params
    {
        std::uint32_t resolution { 128u };   //~ verts per side min two
        float         world_size { 100.0f }; //~ metres across the whole grid
        float         height     { 20.0f };  //~ peak displacement up and down
        std::uint32_t seed       { 1337u };
        float         frequency  { 0.01f };  //~ noise frequency lower is smoother
        int           octaves    { 5 };
        float         lacunarity { 2.0f };
        float         gain       { 0.5f };
    };

    //~ build a grid plane displaced by fractal noise with smooth per vertex
    //~ normals a single submesh and recomputed bounds parametric mountain for now
    //~ TODO: add chunked streaming terrain later
    [[nodiscard]] mesh_data generate_terrain(const terrain_params& params);
} // namespace trishul::render::mesh

#endif //CURSEOFTHESEA_TERRAIN_H
