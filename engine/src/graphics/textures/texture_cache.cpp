// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/textures/texture_cache.h"
#include "engine/utils/helpers.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace cots::graphics::textures
{
    namespace
    {
        template<typename T> void wr(std::ofstream& f, const T& v)
        {
            f.write(reinterpret_cast<const char*>(&v), sizeof(T));
        }
        template<typename T> bool rd(std::ifstream& f, T& v)
        {
            return static_cast<bool>(f.read(reinterpret_cast<char*>(&v), sizeof(T)));
        }
    } //~ anonymous namespace

    bool texture_cache::initialize()
    {
        return baker_.initialize();
    }

    void texture_cache::deinitialize() noexcept
    {
        baker_.deinitialize();
    }

    std::string texture_cache::baked_path_for(const std::string_view source_path)
    {
        //~ derive the baked path
        const std::filesystem::path src(source_path);
        const std::string stem = src.stem().string();

        std::filesystem::path out("compiled");
        out /= "textures";
        out /= stem + ".ctex";
        return out.string();
    }

    bool texture_cache::try_load_cached(const std::string& baked_path,
                                        const std::uint64_t expected_hash,
                                        const texture_intent expected_intent,
                                        baked_blob& out) const
    {
        std::ifstream f(baked_path, std::ios::binary);
        if (!f.is_open()) return false;

        container_header hdr{};
        if (!rd(f, hdr))                                  return false;
        if (hdr.magic        != k_container_magic)        return false;

        //~ file format guard reject wholesale
        if (hdr.file_version != k_container_file_version)
        {
            spdlog::info("[texture-cache] file version mismatch for '{}' got {} want {}",
                         baked_path, hdr.file_version, k_container_file_version);
            return false;
        }

        //~ bake schema guard force rebake
        if (hdr.bake_schema  != k_container_bake_schema)
        {
            spdlog::info("[texture-cache] bake schema mismatch for '{}' got {} want {}",
                         baked_path, hdr.bake_schema, k_container_bake_schema);
            return false;
        }

        if (hdr.source_hash  != expected_hash)
        {
            spdlog::info("[texture-cache] source hash drift for '{}'", baked_path);
            return false;
        }

        if (hdr.intent != static_cast<std::uint32_t>(expected_intent))
        {
            spdlog::info("[texture-cache] intent drift for '{}' got {} want {}",
                         baked_path, hdr.intent,
                         static_cast<std::uint32_t>(expected_intent));
            return false;
        }

        out.dds.resize(static_cast<std::size_t>(hdr.dds_size));
        f.read(reinterpret_cast<char*>(out.dds.data()),
               static_cast<std::streamsize>(hdr.dds_size));
        if (!f)
        {
            spdlog::warn("[texture-cache] short read for '{}'", baked_path);
            out.dds.clear();
            return false;
        }

        out.source_hash = hdr.source_hash;
        out.intent      = expected_intent;
        return true;
    }

    bool texture_cache::write_container(const std::string& baked_path,
                                        const std::uint64_t source_hash,
                                        const texture_intent intent,
                                        const std::vector<std::uint8_t>& dds) const
    {
        std::error_code ec;
        std::filesystem::create_directories(
            std::filesystem::path(baked_path).parent_path(), ec);

        std::ofstream f(baked_path, std::ios::binary | std::ios::trunc);
        if (!f.is_open())
        {
            spdlog::error("[texture-cache] cannot write '{}'", baked_path);
            return false;
        }

        container_header hdr{};
        hdr.magic        = k_container_magic;
        hdr.file_version = k_container_file_version;
        hdr.bake_schema  = k_container_bake_schema;
        hdr.intent       = static_cast<std::uint32_t>(intent);
        hdr.source_hash  = source_hash;
        hdr.dds_size     = dds.size();

        wr(f, hdr);
        f.write(reinterpret_cast<const char*>(dds.data()),
                static_cast<std::streamsize>(dds.size()));
        return static_cast<bool>(f);
    }

    bool texture_cache::get_or_bake(const std::string_view source_path,
                                    const texture_intent intent,
                                    baked_blob& out)
    {
        out = baked_blob{};

        //~ hash the source bytes
        std::string source_bytes;
        if (!helpers::read_file(source_path, source_bytes))
        {
            spdlog::error("[texture-cache] cannot read source '{}'", source_path);
            return false;
        }
        const std::uint64_t shash = helpers::fnv1a(source_bytes);

        const std::string baked_path = baked_path_for(source_path);

        if (try_load_cached(baked_path, shash, intent, out))
        {
            spdlog::info("[texture-cache] hit '{}' -> '{}' {} bytes",
                         source_path, baked_path, out.dds.size());
            return true;
        }

        //~ miss bake then write
        spdlog::info("[texture-cache] miss '{}' baking", source_path);
        std::vector<std::uint8_t> dds;
        if (!baker_.bake(source_path, intent, dds))
        {
            spdlog::error("[texture-cache] bake failed for '{}'", source_path);
            return false;
        }

        if (!write_container(baked_path, shash, intent, dds))
        {
            spdlog::warn("[texture-cache] write failed for '{}' continuing in memory",
                         baked_path);
        }

        out.dds         = std::move(dds);
        out.source_hash = shash;
        out.intent      = intent;
        return true;
    }

    void texture_cache::invalidate(const std::string_view source_path) const
    {
        const std::string baked_path = baked_path_for(source_path);
        std::error_code ec;
        std::filesystem::remove(baked_path, ec);
        if (!ec)
            spdlog::info("[texture-cache] removed '{}'", baked_path);
    }
} // namespace cots::graphics::textures
