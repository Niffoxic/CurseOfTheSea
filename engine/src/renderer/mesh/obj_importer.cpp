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
#include "import_common.h"

#include "trishul/utils/logger.h"

#include <map>
#include <string>
#include <tuple>

#include <tiny_obj_loader.h>

namespace trishul::render::mesh::detail
{
    bool import_obj(const std::string_view path, mesh_data& out, const import_options& opts)
    {
        tinyobj::attrib_t                attrib;
        std::vector<tinyobj::shape_t>    shapes;
        std::vector<tinyobj::material_t> materials;
        std::string                      err;

        const std::string file(path);

        const bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials,
                                         &err, file.c_str(),
                                         nullptr, true);
        if (!err.empty()) LOG_ERROR("[obj] {}", err);
        if (!ok)          return false;

        const bool has_normals = !attrib.normals.empty();
        const bool has_uvs     = !attrib.texcoords.empty();

        //~ obj stores separate indices per attribute weld unique pos normal uv
        //~ tuples into one vertex so the gpu gets a single indexed stream
        std::map<std::tuple<int, int, int>, std::uint32_t> unique;

        for (const tinyobj::shape_t& shape : shapes)
        {
            const std::uint32_t sub_offset = out.index_count();

            for (const tinyobj::index_t& idx : shape.mesh.indices)
            {
                const std::tuple key{ idx.vertex_index, idx.normal_index, idx.texcoord_index };
                auto it = unique.find(key);
                if (it == unique.end())
                {
                    const auto new_index = out.vertex_count();

                    const int vi = idx.vertex_index;
                    out.positions.push_back({
                        attrib.vertices[3 * vi + 0],
                        attrib.vertices[3 * vi + 1],
                        attrib.vertices[3 * vi + 2] });

                    if (has_normals && idx.normal_index >= 0)
                    {
                        const int ni = idx.normal_index;
                        out.normals.push_back({
                            attrib.normals[3 * ni + 0],
                            attrib.normals[3 * ni + 1],
                            attrib.normals[3 * ni + 2] });
                    }

                    if (has_uvs && idx.texcoord_index >= 0)
                    {
                        const int ti = idx.texcoord_index;
                        const float u = attrib.texcoords[2 * ti + 0];
                        float       v = attrib.texcoords[2 * ti + 1];
                        if (opts.flip_uv_v) v = 1.0f - v;
                        out.uvs.push_back({ u, v });
                    }

                    it = unique.emplace(key, new_index).first;
                }
                out.indices.push_back(it->second);
            }

            //~ one submesh per shape material slot from the shapes first face id
            //~ fallback zero when the obj had no material binding
            std::uint32_t material_slot = 0u;
            if (!shape.mesh.material_ids.empty() && shape.mesh.material_ids[0] >= 0)
                material_slot = static_cast<std::uint32_t>(shape.mesh.material_ids[0]);

            const std::uint32_t sub_count = out.index_count() - sub_offset;
            if (sub_count > 0u)
                out.submeshes.push_back(submesh{ sub_offset, sub_count, material_slot, {} });
        }

        //~ if normals were partial drop them so the common path regenerates flat
        if (!out.normals.empty() && out.normals.size() != out.positions.size())
            out.normals.clear();
        if (!out.uvs.empty() && out.uvs.size() != out.positions.size())
            out.uvs.clear();

        return out.vertex_count() > 0u && out.index_count() > 0u;
    }
} // namespace trishul::render::mesh::detail
