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
#ifndef CURSEOFTHESEA_MESH_LOADER_H
#define CURSEOFTHESEA_MESH_LOADER_H

#include <string_view>
#include "trishul/renderer/mesh/mesh_types.h"

namespace trishul::render::mesh
{
    //~ load a cooked cmesh straight off disk
    [[nodiscard]] bool load_mesh(std::string_view cooked_path, mesh_data& out);
} // namespace trishul::render::mesh

#endif //CURSEOFTHESEA_MESH_LOADER_H
