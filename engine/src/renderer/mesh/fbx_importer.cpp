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

#include <string>
#include <vector>
#include <ufbx.h>

namespace trishul::render::mesh::detail
{
    bool import_fbx(const std::string_view path, mesh_data& out, const import_options& opts)
    {
        ufbx_load_opts load_opts{};
        //~ right handed y up
        load_opts.target_axes        = ufbx_axes_right_handed_y_up;
        load_opts.target_unit_meters = 1.0f;
        load_opts.generate_missing_normals = true;

        ufbx_error error;
        const std::string file(path);
        ufbx_scene* scene = ufbx_load_file(file.c_str(), &load_opts, &error);
        if (!scene)
        {
            LOG_ERROR("load failed for '{}' : {}", path, error.description.data);
            return false;
        }

        bool skinned_seen = false;

        for (std::size_t mi = 0; mi < scene->meshes.count; ++mi)
        {
            const ufbx_mesh* mesh = scene->meshes.data[mi];
            if (!mesh || mesh->num_faces == 0) continue;

            if (mesh->skin_deformers.count > 0) skinned_seen = true;

            const bool has_normals = mesh->vertex_normal.exists;
            const bool has_uvs     = mesh->vertex_uv.exists;

            const std::uint32_t sub_offset = out.index_count();

            std::vector<std::uint32_t> tri(mesh->max_face_triangles * 3u);

            for (std::size_t fi = 0; fi < mesh->num_faces; ++fi)
            {
                const ufbx_face face = mesh->faces.data[fi];
                const std::size_t num_tris =
                    ufbx_triangulate_face(tri.data(), tri.size(), mesh, face);

                for (std::size_t c = 0; c < num_tris * 3u; ++c)
                {
                    const std::uint32_t corner = tri[c];
                    const auto idx = out.vertex_count();

                    const ufbx_vec3 p = ufbx_get_vertex_vec3(&mesh->vertex_position, corner);
                    out.positions.push_back({
                        static_cast<float>(p.x),
                        static_cast<float>(p.y),
                        static_cast<float>(p.z) });

                    if (has_normals)
                    {
                        const ufbx_vec3 n = ufbx_get_vertex_vec3(&mesh->vertex_normal, corner);
                        out.normals.push_back({
                            static_cast<float>(n.x),
                            static_cast<float>(n.y),
                            static_cast<float>(n.z) });
                    }

                    if (has_uvs)
                    {
                        const ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, corner);
                        float v = static_cast<float>(uv.y);
                        if (opts.flip_uv_v) v = 1.0f - v;
                        out.uvs.push_back({ static_cast<float>(uv.x), v });
                    }

                    out.indices.push_back(idx);
                }
            }

            //~ one submesh per fbx mesh node material splitting comes later
            const std::uint32_t sub_count = out.index_count() - sub_offset;
            if (sub_count > 0u)
                out.submeshes.push_back(submesh{ sub_offset, sub_count, 0u, {} });
        }

        if (skinned_seen)
            LOG_WARN("'{}' has skin deformers skinned import deferred loading static geometry", path);

        ufbx_free_scene(scene);

        if (!out.normals.empty() && out.normals.size() != out.positions.size())
            out.normals.clear();
        if (!out.uvs.empty() && out.uvs.size() != out.positions.size())
            out.uvs.clear();

        return out.vertex_count() > 0u && out.index_count() > 0u;
    }
} // namespace trishul::render::mesh::detail
