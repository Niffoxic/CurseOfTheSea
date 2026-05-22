// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/meshes/mesh_baker.h"
#include "engine/graphics/meshes/mesh_bake_storage.h"

#include <meshoptimizer.h>
#include <spdlog/spdlog.h>

#include <cstring>
#include <vector>

namespace cots::graphics::meshes
{
    namespace
    {
        //~ expand indices to uint
        std::vector<unsigned int> indices_to_uint(const imported_model& m)
        {
            std::vector<unsigned int> out(m.index_count);
            if (m.index_count == 0) return out;

            if (m.index_16bit)
            {
                const auto* src = reinterpret_cast<const std::uint16_t*>(m.indices.data());
                for (std::uint32_t i = 0; i < m.index_count; ++i)
                    out[i] = src[i];
            }
            else
            {
                const auto* src = reinterpret_cast<const std::uint32_t*>(m.indices.data());
                for (std::uint32_t i = 0; i < m.index_count; ++i)
                    out[i] = src[i];
            }
            return out;
        }

        void uint_to_indices(const std::vector<unsigned int>& src, imported_model& m)
        {
            m.index_count = static_cast<std::uint32_t>(src.size());
            if (m.index_16bit)
            {
                m.indices.resize(src.size() * sizeof(std::uint16_t));
                auto* dst = reinterpret_cast<std::uint16_t*>(m.indices.data());
                for (std::size_t i = 0; i < src.size(); ++i)
                    dst[i] = static_cast<std::uint16_t>(src[i]);
            }
            else
            {
                m.indices.resize(src.size() * sizeof(std::uint32_t));
                auto* dst = reinterpret_cast<std::uint32_t*>(m.indices.data());
                std::memcpy(dst, src.data(), m.indices.size());
            }
        }
    } //~ anonymous namespace

    bool optimize_in_place(imported_model& m)
    {
        if (!m.valid() || m.index_count == 0 || m.streams.empty())
            return false;

        //~ one mesh opt stream per attribute
        std::vector<meshopt_Stream> streams;
        streams.reserve(m.streams.size());
        for (const auto& s : m.streams)
        {
            meshopt_Stream ms{};
            ms.data   = s.bytes.data();
            ms.size   = s.stride;
            ms.stride = s.stride;
            streams.push_back(ms);
        }

        const auto indices = indices_to_uint(m);

        //~ dedup remap for verts
        std::vector<unsigned int> remap(m.vertex_count);
        const std::size_t unique_vc = meshopt_generateVertexRemapMulti(
            remap.data(),
            indices.data(), indices.size(),
            m.vertex_count,
            streams.data(), streams.size());

        spdlog::info("[mesh-baker] dedup {} -> {} verts", m.vertex_count, unique_vc);

        //~ remap each stream
        std::vector<std::vector<std::uint8_t>> new_streams(m.streams.size());
        for (std::size_t i = 0; i < m.streams.size(); ++i)
        {
            new_streams[i].resize(unique_vc * m.streams[i].stride);
            meshopt_remapVertexBuffer(
                new_streams[i].data(),
                m.streams[i].bytes.data(),
                m.vertex_count,
                m.streams[i].stride,
                remap.data());
        }

        std::vector<unsigned int> new_indices(indices.size());
        meshopt_remapIndexBuffer(
            new_indices.data(),
            indices.data(),
            indices.size(),
            remap.data());

        //~ optimize vertex cache then fetch
        meshopt_optimizeVertexCache(
            new_indices.data(),
            new_indices.data(),
            new_indices.size(),
            unique_vc);

        //~ fetch optimization remap
        std::vector<unsigned int> fetch_remap(unique_vc);
        meshopt_optimizeVertexFetchRemap(
            fetch_remap.data(),
            new_indices.data(),
            new_indices.size(),
            unique_vc);

        meshopt_remapIndexBuffer(
            new_indices.data(),
            new_indices.data(),
            new_indices.size(),
            fetch_remap.data());

        for (std::size_t i = 0; i < new_streams.size(); ++i)
        {
            std::vector<std::uint8_t> reordered(new_streams[i].size());
            meshopt_remapVertexBuffer(
                reordered.data(),
                new_streams[i].data(),
                unique_vc,
                m.streams[i].stride,
                fetch_remap.data());
            new_streams[i] = std::move(reordered);
        }

        //~ write back
        m.vertex_count = static_cast<std::uint32_t>(unique_vc);
        for (std::size_t i = 0; i < m.streams.size(); ++i)
            m.streams[i].bytes = std::move(new_streams[i]);
        uint_to_indices(new_indices, m);
        return true;
    }

    bool serialize_mesh(const imported_model& m,
                        const std::uint64_t source_hash,
                        std::vector<std::uint8_t>& out_blob)
    {
        out_blob.clear();

        mesh_container_header hdr{};
        hdr.magic         = k_mesh_container_magic;
        hdr.file_version  = k_mesh_container_file_version;
        hdr.bake_schema   = k_mesh_container_bake_schema;
        hdr.stream_count  = static_cast<std::uint32_t>(m.streams.size());
        hdr.vertex_count  = m.vertex_count;
        hdr.index_count   = m.index_count;
        hdr.index_16bit   = m.index_16bit ? 1u : 0u;
        hdr.index_bytes   = static_cast<std::uint32_t>(m.indices.size());
        hdr.source_hash   = source_hash;

        //~ compute total bytes
        std::size_t total = sizeof(hdr);
        for (const auto& s : m.streams)
        {
            total += sizeof(mesh_stream_record);
            total += s.semantic.size();
            total += s.bytes.size();
        }
        total += m.indices.size();

        out_blob.reserve(total);

        auto push_bytes = [&](const void* p, const std::size_t n)
        {
            const auto* b = static_cast<const std::uint8_t*>(p);
            out_blob.insert(out_blob.end(), b, b + n);
        };

        push_bytes(&hdr, sizeof(hdr));

        for (const auto& s : m.streams)
        {
            mesh_stream_record rec{};
            rec.semantic_len = static_cast<std::uint32_t>(s.semantic.size());
            rec.stride       = s.stride;
            rec.bytes        = static_cast<std::uint32_t>(s.bytes.size());
            rec.reserved     = 0u;

            push_bytes(&rec, sizeof(rec));
            push_bytes(s.semantic.data(), s.semantic.size());
            push_bytes(s.bytes.data(),    s.bytes.size());
        }

        push_bytes(m.indices.data(), m.indices.size());
        return true;
    }

    bool deserialize_mesh(const void* data, const std::size_t size, imported_model& out)
    {
        out = imported_model{};
        if (size < sizeof(mesh_container_header))
            return false;

        const auto* bytes = static_cast<const std::uint8_t*>(data);
        std::size_t off   = 0;

        mesh_container_header hdr{};
        std::memcpy(&hdr, bytes + off, sizeof(hdr));
        off += sizeof(hdr);

        if (hdr.magic        != k_mesh_container_magic)        return false;
        if (hdr.file_version != k_mesh_container_file_version) return false;
        if (hdr.bake_schema  != k_mesh_container_bake_schema)  return false;

        out.vertex_count = hdr.vertex_count;
        out.index_count  = hdr.index_count;
        out.index_16bit  = (hdr.index_16bit != 0u);

        out.streams.resize(hdr.stream_count);
        for (std::uint32_t i = 0; i < hdr.stream_count; ++i)
        {
            if (off + sizeof(mesh_stream_record) > size) return false;
            mesh_stream_record rec{};
            std::memcpy(&rec, bytes + off, sizeof(rec));
            off += sizeof(rec);

            if (off + rec.semantic_len + rec.bytes > size) return false;

            out.streams[i].semantic.assign(
                reinterpret_cast<const char*>(bytes + off), rec.semantic_len);
            off += rec.semantic_len;

            out.streams[i].stride = rec.stride;
            out.streams[i].bytes.assign(bytes + off, bytes + off + rec.bytes);
            off += rec.bytes;
        }

        if (off + hdr.index_bytes > size) return false;
        out.indices.assign(bytes + off, bytes + off + hdr.index_bytes);
        return true;
    }
} // namespace cots::graphics::meshes
