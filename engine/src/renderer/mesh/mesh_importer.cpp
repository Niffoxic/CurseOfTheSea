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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

namespace trishul::render::mesh
{
    namespace
    {
        [[nodiscard]] std::string to_lower(std::string s)
        {
            std::ranges::transform(s, s.begin(),
                [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }
    } // anonymous namespace

    namespace detail
    {
        void generate_flat_normals(mesh_data& m)
        {
            const std::uint32_t vcount = m.vertex_count();
            m.normals.assign(vcount, DirectX::XMFLOAT3{ 0.f, 0.f, 0.f });

            //~ accumulate each triangles geometric normal onto its three verts
            for (std::size_t i = 0; i + 2 < m.indices.size(); i += 3)
            {
                const std::uint32_t a = m.indices[i];
                const std::uint32_t b = m.indices[i + 1];
                const std::uint32_t c = m.indices[i + 2];
                if (a >= vcount || b >= vcount || c >= vcount) continue;

                const auto& pa = m.positions[a];
                const auto& pb = m.positions[b];
                const auto& pc = m.positions[c];
                const DirectX::XMFLOAT3 e0{ pb.x - pa.x, pb.y - pa.y, pb.z - pa.z };
                const DirectX::XMFLOAT3 e1{ pc.x - pa.x, pc.y - pa.y, pc.z - pa.z };
                const DirectX::XMFLOAT3 n{
                    e0.y * e1.z - e0.z * e1.y,
                    e0.z * e1.x - e0.x * e1.z,
                    e0.x * e1.y - e0.y * e1.x };
                for (const std::uint32_t v : { a, b, c })
                {
                    m.normals[v].x += n.x;
                    m.normals[v].y += n.y;
                    m.normals[v].z += n.z;
                }
            }

            for (auto& n : m.normals)
            {
                const float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
                if (len > 1e-8f) { n.x /= len; n.y /= len; n.z /= len; }
                else             { n = { 0.f, 1.f, 0.f }; }
            }
        }

        void finalize_imported(mesh_data& m, const std::string_view debug_name)
        {
            m.format = mesh_format::positions;
            if (!m.normals .empty()) m.format |= mesh_format::normals;
            if (!m.uvs     .empty()) m.format |= mesh_format::uvs;
            if (!m.colors  .empty()) m.format |= mesh_format::colors;
            if (!m.tangents.empty()) m.format |= mesh_format::tangents;
            if (!m.joints  .empty()) m.format |= mesh_format::skinned;

            if (m.submeshes.empty() && m.index_count() > 0u)
                m.submeshes.push_back(submesh{ 0u, m.index_count(), 0u, {} });

            if (m.name.empty()) m.name = std::string(debug_name);
            m.recompute_bounds();
        }
    } // namespace detail

    bool import_mesh(const std::string_view path, mesh_data& out, const import_options& opts)
    {
        out = mesh_data{};

        const std::string ext = to_lower(std::filesystem::path(path).extension().string());

        bool ok = false;
        if      (ext == ".obj")                 ok = detail::import_obj (path, out, opts);
        else if (ext == ".gltf" || ext == ".glb") ok = detail::import_gltf(path, out, opts);
        else if (ext == ".fbx")                 ok = detail::import_fbx (path, out, opts);
        else
        {
            LOG_ERROR("unsupported extension '{}' for '{}'", ext, path);
            return false;
        }

        if (!ok)
        {
            LOG_ERROR("failed to import '{}'", path);
            return false;
        }

        if (opts.generate_normals && out.normals.empty() && !out.indices.empty())
            detail::generate_flat_normals(out);

        detail::finalize_imported(out, std::filesystem::path(path).stem().string());

        if (!out.valid())
        {
            LOG_ERROR("'{}' imported but failed validation", path);
            return false;
        }

        LOG_INFO("'{}' {} verts {} indices {} submeshes",
                 path, out.vertex_count(), out.index_count(),
                 static_cast<std::uint32_t>(out.submeshes.size()));
        return true;
    }
} // namespace trishul::render::mesh
