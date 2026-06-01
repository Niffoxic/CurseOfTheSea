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
#ifndef CURSEOFTHESEA_MESH_BAKER_H
#define CURSEOFTHESEA_MESH_BAKER_H

#include <cstdint>
#include <string>
#include <string_view>

#include "trishul/renderer/mesh/mesh_types.h"

namespace trishul::render::mesh
{
    struct bake_options
    {
        bool optimize     { true };  //~ meshopt vertex cache then fetch reorder
        bool generate_lods{ false }; //~ reserved meshopt simplify hook for later
    };

    //~ import a source file then cook it to out_path
    [[nodiscard]] bool bake_mesh(std::string_view source_path,
                                 std::string_view out_path,
                                 const bake_options& opts = {});

    //~ cook an already built mesh
    [[nodiscard]] bool bake_mesh_data(const mesh_data&    mesh,
                                      std::string_view    out_path,
                                      std::uint64_t       source_hash = 0u,
                                      const bake_options& opts = {});

    //~ cook on a default path
    [[nodiscard]] std::string default_cooked_path(std::string_view source_path);
} // namespace trishul::render::mesh

#endif //CURSEOFTHESEA_MESH_BAKER_H
