// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/meshes/gltf_importer.h"

#include <spdlog/spdlog.h>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <cstring>
#include <filesystem>
#include <vector>

namespace cots::graphics::meshes
{
    namespace
    {
        //~ flip z for left handed
        constexpr float k_z_flip = -1.0f;

        //~ pack floats into bytes
        void to_bytes(const std::vector<float>& src, std::vector<std::uint8_t>& dst)
        {
            dst.resize(src.size() * sizeof(float));
            if (!src.empty())
                std::memcpy(dst.data(), src.data(), dst.size());
        }
    } //~ anonymous namespace

    bool gltf_importer::import_model(const std::string_view source_path,
                                     imported_model& out)
    {
        out = imported_model{};
        out.source_name = std::string(source_path);

        const std::filesystem::path path{ source_path };
        auto data = fastgltf::GltfDataBuffer::FromPath(path);
        if (data.error() != fastgltf::Error::None)
        {
            spdlog::error("[gltf] cannot read '{}'", source_path);
            return false;
        }

        fastgltf::Parser parser{};
        constexpr auto options = fastgltf::Options::LoadExternalBuffers |
                                 fastgltf::Options::DecomposeNodeMatrices;

        auto asset_result = parser.loadGltf(data.get(),
                                            path.parent_path(),
                                            options);
        if (asset_result.error() != fastgltf::Error::None)
        {
            spdlog::error("[gltf] parse failed for '{}'", source_path);
            return false;
        }

        const fastgltf::Asset& asset = asset_result.get();

        if (asset.meshes.empty())
        {
            spdlog::error("[gltf] no meshes in '{}'", source_path);
            return false;
        }

        //~ first mesh first primitive
        const auto& mesh = asset.meshes[0];
        if (mesh.primitives.empty())
        {
            spdlog::error("[gltf] mesh has no primitives");
            return false;
        }
        const auto& primitive = mesh.primitives[0];

        //~ positions
        const auto* pos_it = primitive.findAttribute("POSITION");
        if (pos_it == primitive.attributes.end())
        {
            spdlog::error("[gltf] primitive lacks POSITION");
            return false;
        }

        const auto& pos_acc = asset.accessors[pos_it->accessorIndex];
        out.vertex_count = static_cast<std::uint32_t>(pos_acc.count);
        if (out.vertex_count == 0)
        {
            spdlog::error("[gltf] zero vertices");
            return false;
        }

        //~ position stream
        {
            std::vector<float> positions(static_cast<std::size_t>(out.vertex_count) * 3u);
            std::size_t idx = 0;
            fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                asset, pos_acc,
                [&](fastgltf::math::fvec3 v)
                {
                    positions[idx * 3 + 0] = v[0];
                    positions[idx * 3 + 1] = v[1];
                    positions[idx * 3 + 2] = k_z_flip * v[2];
                    ++idx;
                });

            imported_stream s{};
            s.semantic = "POSITION";
            s.stride   = sizeof(float) * 3u;
            to_bytes(positions, s.bytes);
            out.streams.push_back(std::move(s));
        }

        //~ texcoord stream
        {
            std::vector<float> uvs(static_cast<std::size_t>(out.vertex_count) * 2u, 0.0f);
            if (const auto* uv_it = primitive.findAttribute("TEXCOORD_0");
                uv_it != primitive.attributes.end())
            {
                const auto& uv_acc = asset.accessors[uv_it->accessorIndex];
                std::size_t idx = 0;
                fastgltf::iterateAccessor<fastgltf::math::fvec2>(
                    asset, uv_acc,
                    [&](fastgltf::math::fvec2 v)
                    {
                        uvs[idx * 2 + 0] = v[0];
                        uvs[idx * 2 + 1] = v[1];
                        ++idx;
                    });
            }

            imported_stream s{};
            s.semantic = "TEXCOORD";
            s.stride   = sizeof(float) * 2u;
            to_bytes(uvs, s.bytes);
            out.streams.push_back(std::move(s));
        }

        //~ color stream synth white
        {
            std::vector<float> colors(static_cast<std::size_t>(out.vertex_count) * 3u, 1.0f);
            if (const auto* col_it = primitive.findAttribute("COLOR_0");
                col_it != primitive.attributes.end())
            {
                const auto& col_acc = asset.accessors[col_it->accessorIndex];
                std::size_t idx = 0;
                //~ read as float vec
                fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                    asset, col_acc,
                    [&](fastgltf::math::fvec3 v)
                    {
                        colors[idx * 3 + 0] = v[0];
                        colors[idx * 3 + 1] = v[1];
                        colors[idx * 3 + 2] = v[2];
                        ++idx;
                    });
            }

            imported_stream s{};
            s.semantic = "COLOR";
            s.stride   = sizeof(float) * 3u;
            to_bytes(colors, s.bytes);
            out.streams.push_back(std::move(s));
        }

        //~ normal stream optional
        if (const auto* n_it = primitive.findAttribute("NORMAL");
            n_it != primitive.attributes.end())
        {
            const auto& n_acc = asset.accessors[n_it->accessorIndex];
            std::vector<float> normals(static_cast<std::size_t>(out.vertex_count) * 3u);
            std::size_t idx = 0;
            fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                asset, n_acc,
                [&](fastgltf::math::fvec3 v)
                {
                    normals[idx * 3 + 0] = v[0];
                    normals[idx * 3 + 1] = v[1];
                    normals[idx * 3 + 2] = k_z_flip * v[2];
                    ++idx;
                });

            imported_stream s{};
            s.semantic = "NORMAL";
            s.stride   = sizeof(float) * 3u;
            to_bytes(normals, s.bytes);
            out.streams.push_back(std::move(s));
        }

        //~ indices
        if (primitive.indicesAccessor.has_value())
        {
            const auto& idx_acc = asset.accessors[primitive.indicesAccessor.value()];
            out.index_count = static_cast<std::uint32_t>(idx_acc.count);

            //~ width based on count
            out.index_16bit = (out.vertex_count <= 65535u);

            if (out.index_16bit)
            {
                std::vector<std::uint16_t> tmp(out.index_count);
                std::size_t i = 0;
                fastgltf::iterateAccessor<std::uint32_t>(
                    asset, idx_acc,
                [&](std::uint32_t v)
                {
                    tmp[i++] = static_cast<std::uint16_t>(v);
                });

                out.indices.resize(tmp.size() * sizeof(std::uint16_t));
                std::memcpy(out.indices.data(), tmp.data(), out.indices.size());
            }
            else
            {
                std::vector<std::uint32_t> tmp(out.index_count);
                std::size_t i = 0;
                fastgltf::iterateAccessor<std::uint32_t>(
                    asset, idx_acc,
                    [&](const std::uint32_t v)
                    {
                        tmp[i++] = v;
                    });

                out.indices.resize(tmp.size() * sizeof(std::uint32_t));
                std::memcpy(out.indices.data(), tmp.data(), out.indices.size());
            }
        }

        spdlog::info("[gltf] '{}' loaded {} verts {} indices {} streams",
                     source_path, out.vertex_count, out.index_count, out.streams.size());
        return true;
    }
} // namespace cots::graphics::meshes
