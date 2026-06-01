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
#ifndef CURSEOFTHESEA_MESH_IMPORTER_H
#define CURSEOFTHESEA_MESH_IMPORTER_H

#include <string_view>
#include "trishul/renderer/mesh/mesh_types.h"

namespace trishul::render::mesh
{
    struct import_options
    {
        bool flip_uv_v        { true };
        bool generate_normals { true };
    };

    //~ load a source mesh into out picks the importer by file extension supports
    //~ gltf glb obj fbx returns false on an unknown extension
    [[nodiscard]] bool import_mesh(std::string_view path,
                                   mesh_data&       out,
                                   const import_options& opts = {});
} // namespace trishul::render::mesh

#endif //CURSEOFTHESEA_MESH_IMPORTER_H
