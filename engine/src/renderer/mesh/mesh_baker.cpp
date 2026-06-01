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
#include "trishul/renderer/mesh/mesh_baker.h"
#include "trishul/renderer/mesh/mesh_baked_storage.h"
#include "trishul/renderer/mesh/mesh_importer.h"
#include "trishul/utils/logger.h"
#include "trishul/utils/statics.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <type_traits>
#include <vector>

#include <meshoptimizer.h>

namespace trishul::render::mesh
{
    namespace
    {
        template<typename T> void wr(std::ofstream& f, const T& v)
        {
            f.write(reinterpret_cast<const char*>(&v), sizeof(T));
        }
        template<typename T> void wr_array(std::ofstream& f, const std::vector<T>& v)
        {
            if (v.empty()) return;
            f.write(reinterpret_cast<const char*>(v.data()),
                    static_cast<std::streamsize>(v.size() * sizeof(T)));
        }

        //~ run the meshopt passes per submesh slice
        void optimize(mesh_data& m)
        {
            const std::size_t vc = m.positions.size();
            if (vc == 0u || m.indices.empty()) return;

            for (const submesh& sm : m.submeshes)
            {
                if (sm.index_count == 0u) continue;
                if (static_cast<std::uint64_t>(sm.index_offset) + sm.index_count > m.indices.size())
                    continue;
                meshopt_optimizeVertexCache(
                    m.indices.data() + sm.index_offset,
                    m.indices.data() + sm.index_offset,
                    sm.index_count, vc);
            }

            std::vector<unsigned int> remap(vc);
            meshopt_optimizeVertexFetchRemap(
                remap.data(), m.indices.data(), m.indices.size(), vc);

            meshopt_remapIndexBuffer(
                m.indices.data(), m.indices.data(), m.indices.size(), remap.data());

            auto remap_stream = [&](auto& vec)
            {
                if (vec.size() != vc) return;
                using elem = std::decay_t<decltype(vec[0])>;
                std::vector<elem> tmp(vc);
                meshopt_remapVertexBuffer(tmp.data(), vec.data(), vc, sizeof(elem), remap.data());
                vec.swap(tmp);
            };
            remap_stream(m.positions);
            remap_stream(m.normals);
            remap_stream(m.uvs);
            remap_stream(m.colors);
            remap_stream(m.tangents);
            remap_stream(m.joints);
            remap_stream(m.weights);
        }

        [[nodiscard]] std::uint64_t payload_size(const mesh_data& m)
        {
            const std::uint64_t vc = m.vertex_count();
            std::uint64_t bytes = vc * sizeof(DirectX::XMFLOAT3); //~ positions
            if (has_flag(m.format, mesh_format::normals))  bytes += vc * sizeof(DirectX::XMFLOAT3);
            if (has_flag(m.format, mesh_format::uvs))      bytes += vc * sizeof(DirectX::XMFLOAT2);
            if (has_flag(m.format, mesh_format::colors))   bytes += vc * sizeof(DirectX::XMFLOAT3);
            if (has_flag(m.format, mesh_format::tangents)) bytes += vc * sizeof(DirectX::XMFLOAT4);
            if (has_flag(m.format, mesh_format::skinned))
                bytes += vc * (sizeof(joint_indices) + sizeof(DirectX::XMFLOAT4));
            bytes += static_cast<std::uint64_t>(m.index_count()) * sizeof(std::uint32_t);
            bytes += static_cast<std::uint64_t>(m.submeshes.size()) * sizeof(mesh_submesh_record);
            return bytes;
        }
    } // anonymous namespace

    std::string default_cooked_path(const std::string_view source_path)
    {
        const std::filesystem::path src(source_path);
        const std::string  stem = src.stem().string();
        const std::uint64_t tag = statics::fnv1a(source_path);

        std::filesystem::path out("compiled");
        out /= "meshes";
        out /= std::format("{}_{:016x}.cmesh", stem, tag);
        return out.string();
    }

    bool bake_mesh_data(const mesh_data& mesh, const std::string_view out_path,
                        const std::uint64_t source_hash, const bake_options& opts)
    {
        if (!mesh.valid())
        {
            LOG_ERROR("mesh failed validation cannot cook '{}'", out_path);
            return false;
        }

        //~ work on a copy so the meshopt reorder does not touch the callers mesh
        mesh_data cooked = mesh;
        if (opts.optimize) optimize(cooked);
        cooked.recompute_bounds();

        std::error_code ec;
        std::filesystem::create_directories(
            std::filesystem::path(out_path).parent_path(), ec);

        std::ofstream f(std::string(out_path), std::ios::binary | std::ios::trunc);
        if (!f.is_open())
        {
            LOG_ERROR("cannot write '{}'", out_path);
            return false;
        }

        mesh_container_header hdr{};
        hdr.magic         = k_mesh_magic;
        hdr.file_version  = k_mesh_file_version;
        hdr.bake_schema   = k_mesh_bake_schema;
        hdr.format_flags  = static_cast<std::uint32_t>(cooked.format);
        hdr.vertex_count  = cooked.vertex_count();
        hdr.index_count   = cooked.index_count();
        hdr.submesh_count = static_cast<std::uint32_t>(cooked.submeshes.size());
        hdr.reserved      = 0u;
        hdr.source_hash   = source_hash;
        hdr.payload_size  = payload_size(cooked);
        hdr.bounds_min[0] = cooked.bounds.min.x;
        hdr.bounds_min[1] = cooked.bounds.min.y;
        hdr.bounds_min[2] = cooked.bounds.min.z;
        hdr.bounds_max[0] = cooked.bounds.max.x;
        hdr.bounds_max[1] = cooked.bounds.max.y;
        hdr.bounds_max[2] = cooked.bounds.max.z;
        wr(f, hdr);

        //~ streams in flag order the loader reads them back the exact same way
        wr_array(f, cooked.positions);
        if (has_flag(cooked.format, mesh_format::normals))  wr_array(f, cooked.normals);
        if (has_flag(cooked.format, mesh_format::uvs))      wr_array(f, cooked.uvs);
        if (has_flag(cooked.format, mesh_format::colors))   wr_array(f, cooked.colors);
        if (has_flag(cooked.format, mesh_format::tangents)) wr_array(f, cooked.tangents);
        if (has_flag(cooked.format, mesh_format::skinned))
        {
            wr_array(f, cooked.joints);
            wr_array(f, cooked.weights);
        }
        wr_array(f, cooked.indices);

        for (const submesh& sm : cooked.submeshes)
        {
            mesh_submesh_record rec{};
            rec.index_offset  = sm.index_offset;
            rec.index_count   = sm.index_count;
            rec.material_slot = sm.material_slot;
            rec.bounds_min[0] = sm.bounds.min.x; rec.bounds_min[1] = sm.bounds.min.y; rec.bounds_min[2] = sm.bounds.min.z;
            rec.bounds_max[0] = sm.bounds.max.x; rec.bounds_max[1] = sm.bounds.max.y; rec.bounds_max[2] = sm.bounds.max.z;
            wr(f, rec);
        }

        if (!f)
        {
            LOG_ERROR("write error for '{}'", out_path);
            return false;
        }

        LOG_INFO("cooked '{}' {} verts {} indices",
                 out_path, cooked.vertex_count(), cooked.index_count());
        return true;
    }

    bool bake_mesh(const std::string_view source_path, const std::string_view out_path,
                   const bake_options& opts)
    {
        //~ hash the source bytes so a future tool can tell when the cook is stale
        std::string source_bytes;
        std::uint64_t shash = 0u;
        if (statics::read_file(source_path, source_bytes))
            shash = statics::fnv1a(source_bytes);
        else
            LOG_WARN("could not read '{}' for hashing carrying on", source_path);

        mesh_data imported;
        if (!import_mesh(source_path, imported))
        {
            LOG_ERROR("import failed for '{}'", source_path);
            return false;
        }

        return bake_mesh_data(imported, out_path, shash, opts);
    }
} // namespace trishul::render::mesh
