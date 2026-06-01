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
#include "trishul/renderer/mesh/mesh_loader.h"
#include "trishul/renderer/mesh/mesh_baked_storage.h"
#include "trishul/utils/logger.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace trishul::render::mesh
{
    namespace
    {
        //~ refuse a wildly sized cooked file before trusting its counts we run no
        //~ exceptions so a blind resize on garbage would just take us down
        constexpr std::uint64_t k_max_payload    = 1024ull * 1024ull * 1024ull; //~ 1 GB
        constexpr std::uint32_t k_max_vertices   = 64u * 1024u * 1024u;
        constexpr std::uint32_t k_max_indices    = 256u * 1024u * 1024u;
        constexpr std::uint32_t k_max_submeshes  = 1u << 20;

        template<typename T> bool rd(std::ifstream& f, T& v)
        {
            return static_cast<bool>(f.read(reinterpret_cast<char*>(&v), sizeof(T)));
        }

        //~ pull count elements straight into a typed vector checking the stream
        template<typename T>
        bool rd_array(std::ifstream& f, std::vector<T>& out, const std::uint32_t count)
        {
            out.resize(count);
            if (count == 0u) return true;
            return static_cast<bool>(
                f.read(reinterpret_cast<char*>(out.data()),
                       static_cast<std::streamsize>(static_cast<std::uint64_t>(count) * sizeof(T))));
        }
    } // anonymous namespace

    bool load_mesh(const std::string_view cooked_path, mesh_data& out)
    {
        out = mesh_data{};

        std::ifstream f(std::string(cooked_path), std::ios::binary);
        if (!f.is_open())
        {
            LOG_ERROR("cannot open '{}'", cooked_path);
            return false;
        }

        mesh_container_header hdr{};
        if (!rd(f, hdr))                            return false;
        if (hdr.magic != k_mesh_magic)              { LOG_ERROR("bad magic '{}'", cooked_path); return false; }
        if (hdr.file_version != k_mesh_file_version){ LOG_INFO ("file version mismatch '{}' got {} want {}", cooked_path, hdr.file_version, k_mesh_file_version); return false; }
        if (hdr.bake_schema  != k_mesh_bake_schema) { LOG_INFO ("bake schema mismatch '{}' got {} want {}",  cooked_path, hdr.bake_schema,  k_mesh_bake_schema);  return false; }

        //~ sanity caps before believing any count
        if (hdr.payload_size  > k_max_payload  ||
            hdr.vertex_count  > k_max_vertices ||
            hdr.index_count   > k_max_indices  ||
            hdr.submesh_count > k_max_submeshes)
        {
            LOG_WARN("header counts past the cap for '{}'", cooked_path);
            return false;
        }

        const auto fmt = static_cast<mesh_format>(hdr.format_flags);
        const std::uint32_t vc = hdr.vertex_count;

        //~ streams come back in the same flag order the baker wrote positions are
        //~ always present the rest gated on the format bits
        if (!rd_array(f, out.positions, vc)) return false;
        if (has_flag(fmt, mesh_format::normals)  && !rd_array(f, out.normals,  vc)) return false;
        if (has_flag(fmt, mesh_format::uvs)      && !rd_array(f, out.uvs,      vc)) return false;
        if (has_flag(fmt, mesh_format::colors)   && !rd_array(f, out.colors,   vc)) return false;
        if (has_flag(fmt, mesh_format::tangents) && !rd_array(f, out.tangents, vc)) return false;
        if (has_flag(fmt, mesh_format::skinned))
        {
            if (!rd_array(f, out.joints,  vc)) return false;
            if (!rd_array(f, out.weights, vc)) return false;
        }

        if (!rd_array(f, out.indices, hdr.index_count)) return false;

        out.submeshes.reserve(hdr.submesh_count);
        for (std::uint32_t i = 0; i < hdr.submesh_count; ++i)
        {
            mesh_submesh_record rec{};
            if (!rd(f, rec)) return false;
            submesh sm{};
            sm.index_offset  = rec.index_offset;
            sm.index_count   = rec.index_count;
            sm.material_slot = rec.material_slot;
            sm.bounds.min = { rec.bounds_min[0], rec.bounds_min[1], rec.bounds_min[2] };
            sm.bounds.max = { rec.bounds_max[0], rec.bounds_max[1], rec.bounds_max[2] };
            out.submeshes.push_back(sm);
        }

        out.format = fmt;
        //~ name is not in the container so take it from the file stem
        out.name   = std::filesystem::path(cooked_path).stem().string();
        out.bounds.min = { hdr.bounds_min[0], hdr.bounds_min[1], hdr.bounds_min[2] };
        out.bounds.max = { hdr.bounds_max[0], hdr.bounds_max[1], hdr.bounds_max[2] };

        //~ positions are authoritative so recompute the sphere and submesh boxes
        out.recompute_bounds();

        if (!out.valid())
        {
            LOG_ERROR("'{}' failed validation after load", cooked_path);
            out = mesh_data{};
            return false;
        }

        LOG_INFO("'{}' {} verts {} indices", cooked_path, out.vertex_count(), out.index_count());
        return true;
    }
} // namespace trishul::render::mesh
