// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/shaders/storage/binary_storage.h"

#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>

namespace cots::graphics::shaders
{
    namespace
    {
        constexpr std::uint32_t k_magic   = 0x31435343;
        constexpr std::uint32_t k_version = 1;

        template<typename T> void wr(std::ofstream& f, const T& v)
        {
            f.write(reinterpret_cast<const char*>(&v), sizeof(T));
        }
        template<typename T> bool rd(std::ifstream& f, T& v)
        {
            return static_cast<bool>(f.read(reinterpret_cast<char*>(&v), sizeof(T)));
        }
    }

    bool binary_shader_storage::write_entry(std::ofstream& f, const shader_cache_entry& e)
    {
        wr(f, e.key);
        wr(f, e.source_hash);
        wr(f, static_cast<std::uint32_t>(e.identifier.size()));
        f.write(e.identifier.data(), static_cast<std::streamsize>(e.identifier.size()));
        wr(f, static_cast<std::uint32_t>(e.dxil.size()));
        f.write(reinterpret_cast<const char*>(e.dxil.data()),
                static_cast<std::streamsize>(e.dxil.size()));
        return static_cast<bool>(f);
    }

    bool binary_shader_storage::load_all(cache_map& out)
    {
        out.clear();
        std::ifstream f(path_, std::ios::binary);
        if (!f.is_open()) return true;   //~ cold cache

        std::uint32_t magic = 0, version = 0;
        if (!rd(f, magic) || magic != k_magic)
        {
            spdlog::warn("[shader] bad cache magic"); return true;
        }
        if (!rd(f, version) || version != k_version)
        {
            spdlog::warn("[shader] cache version mismatch"); return true;
        }

        //~ stored count is a hint only
        std::uint32_t hint = 0;
        rd(f, hint);

        while (f.peek() != EOF)
        {
            shader_cache_entry e{};
            std::uint32_t id_len = 0, dxil_len = 0;
            if (!rd(f, e.key) || !rd(f, e.source_hash) || !rd(f, id_len)) break;
            e.identifier.resize(id_len);
            f.read(e.identifier.data(), id_len);
            if (!rd(f, dxil_len)) break;
            e.dxil.resize(dxil_len);
            f.read(reinterpret_cast<char*>(e.dxil.data()), dxil_len);
            if (!f) break;
            out[e.key] = std::move(e);   //~ appended duplicate overwrites
        }
        spdlog::info("[shader] loaded {} cached shader(s) (binary)", out.size());
        return true;
    }

    bool binary_shader_storage::store_all(const cache_map& in)
    {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path_).parent_path(), ec);

        std::ofstream f(path_, std::ios::binary | std::ios::trunc);
        if (!f.is_open()) { spdlog::error("[shader] cannot write {}", path_); return false; }

        wr(f, k_magic);
        wr(f, k_version);
        wr(f, static_cast<std::uint32_t>(in.size()));
        for (const auto& [key, e] : in)
            if (!write_entry(f, e)) return false;

        spdlog::info("[shader] compacted {} shader(s) -> {}", in.size(), path_);
        return true;
    }

    bool binary_shader_storage::ensure_header()
    {
        if (std::filesystem::exists(path_)) return true;

        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path_).parent_path(), ec);

        std::ofstream f(path_, std::ios::binary | std::ios::trunc);
        if (!f.is_open()) return false;
        wr(f, k_magic);
        wr(f, k_version);
        wr(f, static_cast<std::uint32_t>(0));   //~ count hint = 0 load reads to EOF anyway
        return true;
    }

    bool binary_shader_storage::store_one(const shader_cache_entry& e, const cache_map&)
    {
        if (!ensure_header())
        {
            spdlog::error("[shader] cannot create {}", path_); return false;
        }

        std::ofstream f(path_, std::ios::binary | std::ios::app);
        if (!f.is_open())
        {
            spdlog::error("[shader] cannot append {}", path_); return false;
        }

        if (!write_entry(f, e))
        {
            spdlog::error("[shader] append failed"); return false;
        }
        f.flush();
        return true;
    }
} // namespace cots::graphics::shaders
