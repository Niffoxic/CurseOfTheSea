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

#include <filesystem>
#include <string>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

namespace trishul::render::mesh::detail
{
    bool import_gltf(const std::string_view path, mesh_data& out, const import_options& opts)
    {
        const std::filesystem::path file(path);

        auto data = fastgltf::GltfDataBuffer::FromPath(file);
        if (data.error() != fastgltf::Error::None)
        {
            LOG_ERROR("cannot open '{}'", path);
            return false;
        }

        constexpr auto options = fastgltf::Options::LoadExternalBuffers
                               | fastgltf::Options::GenerateMeshIndices;

        fastgltf::Parser parser;
        auto loaded = parser.loadGltf(data.get(), file.parent_path(), options);
        if (loaded.error() != fastgltf::Error::None)
        {
            LOG_ERROR("parse failed for '{}'", path);
            return false;
        }
        const fastgltf::Asset& asset = loaded.get();

        bool skinned_seen = false;

        for (const fastgltf::Mesh& mesh : asset.meshes)
        {
            for (const fastgltf::Primitive& prim : mesh.primitives)
            {
                if (prim.type != fastgltf::PrimitiveType::Triangles) continue;

                const auto* pos = prim.findAttribute("POSITION");
                if (pos == prim.attributes.end()) continue;
                if (!prim.indicesAccessor.has_value()) continue;

                if (prim.findAttribute("JOINTS_0") != prim.attributes.end())
                    skinned_seen = true;

                const auto base_vertex = out.vertex_count();
                const auto sub_offset  = out.index_count();

                //~ positions are mandatory weld nothing gltf is already indexed
                const auto& pacc = asset.accessors[pos->accessorIndex];
                fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, pacc,
                    [&](const fastgltf::math::fvec3 v)
                    { out.positions.push_back({ v[0], v[1], v[2] }); });

                if (const auto* nrm = prim.findAttribute("NORMAL");
                    nrm != prim.attributes.end())
                {
                    fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                        asset, asset.accessors[nrm->accessorIndex],
                        [&](const fastgltf::math::fvec3 v)
                        { out.normals.push_back({ v[0], v[1], v[2] }); });
                }

                if (const auto* uv = prim.findAttribute("TEXCOORD_0");
                    uv != prim.attributes.end())
                {
                    fastgltf::iterateAccessor<fastgltf::math::fvec2>(
                        asset, asset.accessors[uv->accessorIndex],
                        [&](const fastgltf::math::fvec2 v)
                        {
                            float vv = v[1];
                            if (opts.flip_uv_v) vv = 1.0f - vv;
                            out.uvs.push_back({ v[0], vv });
                        });
                }

                //~ indices rebased onto whatever vertex range this primitive landed
                const auto& iacc = asset.accessors[prim.indicesAccessor.value()];
                fastgltf::iterateAccessor<std::uint32_t>(asset, iacc,
                    [&](const std::uint32_t idx)
                    { out.indices.push_back(base_vertex + idx); });

                std::uint32_t material_slot = 0u;
                if (prim.materialIndex.has_value())
                    material_slot = static_cast<std::uint32_t>(prim.materialIndex.value());

                const auto sub_count = out.index_count() - sub_offset;
                if (sub_count > 0u)
                    out.submeshes.push_back(submesh{ sub_offset, sub_count, material_slot, {} });
            }
        }

        if (skinned_seen)
            LOG_WARN("'{}' has skin data skinned import deferred loading static geometry", path);

        //~ partial normal or uv coverage across primitives drop them so the
        //~ common path regenerates a clean flat set
        if (!out.normals.empty() && out.normals.size() != out.positions.size())
            out.normals.clear();
        if (!out.uvs.empty() && out.uvs.size() != out.positions.size())
            out.uvs.clear();

        return out.vertex_count() > 0u && out.index_count() > 0u;
    }
} // namespace trishul::render::mesh::detail
