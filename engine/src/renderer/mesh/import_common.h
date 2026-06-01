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
#ifndef CURSEOFTHESEA_IMPORT_COMMON_H
#define CURSEOFTHESEA_IMPORT_COMMON_H

#include <string_view>

#include "trishul/renderer/mesh/mesh_importer.h"
#include "trishul/renderer/mesh/mesh_types.h"

namespace trishul::render::mesh::detail
{
    [[nodiscard]] bool import_obj (std::string_view path, mesh_data& out, const import_options& opts);
    [[nodiscard]] bool import_gltf(std::string_view path, mesh_data& out, const import_options& opts);
    [[nodiscard]] bool import_fbx (std::string_view path, mesh_data& out, const import_options& opts);

    void generate_flat_normals(mesh_data& m);
    void finalize_imported    (mesh_data& m, std::string_view debug_name);
} // namespace trishul::render::mesh::detail

#endif //CURSEOFTHESEA_IMPORT_COMMON_H
