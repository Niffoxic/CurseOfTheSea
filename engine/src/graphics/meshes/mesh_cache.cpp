// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/meshes/mesh_cache.h"
#include "engine/graphics/meshes/mesh_baker.h"
#include "engine/graphics/meshes/mesh_bake_storage.h"
#include "engine/utils/helpers.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace cots::graphics::meshes
{
    bool mesh_cache::initialize(std::unique_ptr<model_importer> imp)
    {
        importer_ = std::move(imp);
        return importer_ != nullptr;
    }

    void mesh_cache::deinitialize() noexcept
    {
        importer_.reset();
    }

    std::string mesh_cache::baked_path_for(const std::string_view source_path)
    {
        //~ derive baked path
        const std::filesystem::path src(source_path);
        const std::string stem = src.stem().string();

        std::filesystem::path out("compiled");
        out /= "meshes";
        out /= stem + ".cmsh";
        return out.string();
    }

    bool mesh_cache::try_load_cached(const std::string& baked_path,
                                     const std::uint64_t expected_hash,
                                     imported_model& out) const
    {
        std::ifstream f(baked_path, std::ios::binary | std::ios::ate);
        if (!f.is_open()) return false;

        const std::streamsize size = f.tellg();
        if (size <= 0) return false;
        f.seekg(0);

        std::vector<std::uint8_t> buf(static_cast<std::size_t>(size));
        f.read(reinterpret_cast<char*>(buf.data()), size);
        if (!f) return false;

        if (buf.size() < sizeof(mesh_container_header))
            return false;

        mesh_container_header hdr{};
        std::memcpy(&hdr, buf.data(), sizeof(hdr));

        if (hdr.magic        != k_mesh_container_magic)
        {
            spdlog::warn("[mesh-cache] bad magic for '{}'", baked_path);
            return false;
        }
        if (hdr.file_version != k_mesh_container_file_version)
        {
            spdlog::info("[mesh-cache] file version mismatch for '{}' got {} want {}",
                         baked_path, hdr.file_version, k_mesh_container_file_version);
            return false;
        }
        if (hdr.bake_schema  != k_mesh_container_bake_schema)
        {
            spdlog::info("[mesh-cache] bake schema mismatch for '{}'", baked_path);
            return false;
        }
        if (hdr.source_hash != expected_hash)
        {
            spdlog::info("[mesh-cache] source hash drift for '{}'", baked_path);
            return false;
        }

        if (!deserialize_mesh(buf.data(), buf.size(), out))
        {
            spdlog::warn("[mesh-cache] deserialize failed for '{}'", baked_path);
            return false;
        }
        return true;
    }

    bool mesh_cache::write_container(const std::string& baked_path,
                                     const std::vector<std::uint8_t>& blob) const
    {
        std::error_code ec;
        std::filesystem::create_directories(
            std::filesystem::path(baked_path).parent_path(), ec);

        std::ofstream f(baked_path, std::ios::binary | std::ios::trunc);
        if (!f.is_open())
        {
            spdlog::error("[mesh-cache] cannot write '{}'", baked_path);
            return false;
        }

        f.write(reinterpret_cast<const char*>(blob.data()),
                static_cast<std::streamsize>(blob.size()));
        return static_cast<bool>(f);
    }

    bool mesh_cache::get_or_bake(const std::string_view source_path,
                                 imported_model& out) const
    {
        out = imported_model{};
        if (!importer_) return false;

        //~ hash the source bytes
        std::string source_bytes;
        if (!helpers::read_file(source_path, source_bytes))
        {
            spdlog::error("[mesh-cache] cannot read source '{}'", source_path);
            return false;
        }
        const std::uint64_t shash = helpers::fnv1a(source_bytes);

        const std::string baked_path = baked_path_for(source_path);

        if (try_load_cached(baked_path, shash, out))
        {
            out.source_name = std::string(source_path);
            spdlog::info("[mesh-cache] hit '{}' -> '{}' {} verts {} indices",
                         source_path, baked_path, out.vertex_count, out.index_count);
            return true;
        }

        //~ miss import then bake
        spdlog::info("[mesh-cache] miss '{}' importing", source_path);
        imported_model raw{};
        if (!importer_->import_model(source_path, raw))
        {
            spdlog::error("[mesh-cache] import failed for '{}'", source_path);
            return false;
        }

        spdlog::info("[mesh-cache] optimizing '{}'", source_path);
        if (!optimize_in_place(raw))
        {
            spdlog::warn("[mesh-cache] optimize failed for '{}' using raw", source_path);
        }

        std::vector<std::uint8_t> blob;
        if (!serialize_mesh(raw, shash, blob))
        {
            spdlog::error("[mesh-cache] serialize failed for '{}'", source_path);
            return false;
        }

        if (!write_container(baked_path, blob))
        {
            spdlog::warn("[mesh-cache] write failed for '{}' continuing in memory",
                         baked_path);
        }

        out = std::move(raw);
        out.source_name = std::string(source_path);
        return true;
    }

    void mesh_cache::invalidate(const std::string_view source_path) const
    {
        const std::string baked_path = baked_path_for(source_path);
        std::error_code ec;
        std::filesystem::remove(baked_path, ec);
        if (!ec)
            spdlog::info("[mesh-cache] removed '{}'", baked_path);
    }
} // namespace cots::graphics::meshes
