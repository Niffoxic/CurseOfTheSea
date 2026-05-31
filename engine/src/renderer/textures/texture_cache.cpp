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
#include "trishul/renderer/textures/texture_cache.h"

#include "trishul/utils/logger.h"
#include "trishul/utils/statics.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <string>

namespace trishul::render::textures
{
    namespace
    {
        //~ refusing a wildly sized payload from a corrupt or truncated container
        //~ we run no exceptions so a blind resize on garbage would just kill us
        constexpr std::uint64_t k_max_dds_bytes = 512ull * 1024ull * 1024ull; //~ 512 MB

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
        //~ deriving the baked path the stem alone collides when two folders hold
        //~ the same file name so we fold a hash of the full path into the name
        //~ to keep distinct sources in their own slot
        const std::filesystem::path src(source_path);
        const std::string stem = src.stem().string();
        const std::uint64_t tag = statics::fnv1a(source_path);

        std::filesystem::path out("compiled");
        out /= "textures";
        out /= std::format("{}_{:016x}.ctex", stem, tag);
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

        //~ file format guard wrong version means we cannot trust the bytes
        if (hdr.file_version != k_container_file_version)
        {
            LOG_INFO("[texture-cache] file version mismatch for '{}' got {} want {}",
                     baked_path, hdr.file_version, k_container_file_version);
            return false;
        }

        //~ bake schema guard the recipe changed so force a rebake
        if (hdr.bake_schema  != k_container_bake_schema)
        {
            LOG_INFO("[texture-cache] bake schema mismatch for '{}' got {} want {}",
                     baked_path, hdr.bake_schema, k_container_bake_schema);
            return false;
        }

        if (hdr.source_hash  != expected_hash)
        {
            LOG_INFO("[texture-cache] source hash drift for '{}'", baked_path);
            return false;
        }

        if (hdr.intent != static_cast<std::uint32_t>(expected_intent))
        {
            LOG_INFO("[texture-cache] intent drift for '{}' got {} want {}",
                     baked_path, hdr.intent,
                     static_cast<std::uint32_t>(expected_intent));
            return false;
        }

        //~ sanity cap before we trust the size and allocate
        if (hdr.dds_size > k_max_dds_bytes)
        {
            LOG_WARN("[texture-cache] dds size {} past the cap for '{}'",
                     hdr.dds_size, baked_path);
            return false;
        }

        out.dds.resize(static_cast<std::size_t>(hdr.dds_size));
        f.read(reinterpret_cast<char*>(out.dds.data()),
               static_cast<std::streamsize>(hdr.dds_size));
        if (!f)
        {
            LOG_WARN("[texture-cache] short read for '{}'", baked_path);
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
            LOG_ERROR("[texture-cache] cannot write '{}'", baked_path);
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
                                    baked_blob& out,
                                    const bake_options& opts)
    {
        out = baked_blob{};

        //~ hashing the source bytes so any edit to the file busts the cache
        std::string source_bytes;
        if (!statics::read_file(source_path, source_bytes))
        {
            LOG_ERROR("[texture-cache] cannot read source '{}'", source_path);
            return false;
        }
        const std::uint64_t shash = statics::fnv1a(source_bytes);

        const std::string baked_path = baked_path_for(source_path);

        if (try_load_cached(baked_path, shash, intent, out))
        {
            LOG_INFO("[texture-cache] hit '{}' -> '{}' {} bytes",
                     source_path, baked_path, out.dds.size());
            return true;
        }

        //~ miss so bake then stash it next to the others
        LOG_INFO("[texture-cache] miss '{}' baking", source_path);
        std::vector<std::uint8_t> dds;
        if (!baker_.bake(source_path, intent, dds, opts))
        {
            LOG_ERROR("[texture-cache] bake failed for '{}'", source_path);
            return false;
        }

        if (!write_container(baked_path, shash, intent, dds))
        {
            LOG_WARN("[texture-cache] write failed for '{}' carrying on in memory",
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
            LOG_INFO("[texture-cache] removed '{}'", baked_path);
    }
} // namespace trishul::render::textures
