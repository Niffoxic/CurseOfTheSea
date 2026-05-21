// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/shaders/shader_cache.h"

#include <cots/cots_config.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>

namespace cots::graphics::shaders
{
    namespace
    {
        std::uint64_t fnv1a(std::string_view s) noexcept
        {
            std::uint64_t h = 0xcbf29ce484222325ull;
            for (const unsigned char c : s) { h ^= c; h *= 0x100000001b3ull; }
            return h;
        }

        const char* cfg_tag() noexcept
        {
#if COTS_DEBUG
            return "dbg";
#else
            return "rel";
#endif
        }

        const char* stage_tag(shader_stage s) noexcept
        {
            switch (s)
            {
                case shader_stage::vertex:  return "vs";
                case shader_stage::pixel:   return "ps";
                case shader_stage::compute: return "cs";
                default:                    return "xx";
            }
        }

        bool read_file(std::string_view path, std::string& out)
        {
            std::ifstream f(std::string(path), std::ios::binary);
            if (!f.is_open()) return false;
            std::ostringstream ss; ss << f.rdbuf();
            out = ss.str();
            return true;
        }
    }

    shader_cache::~shader_cache() { deinitialize(); }

    bool shader_cache::initialize(std::unique_ptr<shader_storage> storage)
    {
        if (!compiler_.initialize()) return false;
        storage_ = std::move(storage);
        if (storage_ && !storage_->load_all(entries_))
            spdlog::warn("[shader] cache load failed - starting cold");
        return true;
    }

    void shader_cache::deinitialize() noexcept
    {
        flush();
        storage_.reset();
        entries_.clear();
        compiler_.deinitialize();
    }

    void shader_cache::flush()
    {
        if (dirty_ && storage_)
        {
            storage_->store_all(entries_);
            dirty_ = false;
        }
    }

    shader_bytecode shader_cache::get_or_compile(
        std::string_view path, std::string_view entry, shader_stage stage)
    {
        std::string source;
        if (!read_file(path, source))
        {
            spdlog::error("[shader] cannot read {}", path);
            return {};
        }

        //~ slot = identifier hash (stable per variant); source_hash validates freshness
        std::string identifier;
        identifier.reserve(path.size() + 16);
        identifier.append(path).append(":").append(entry).append(":")
                  .append(stage_tag(stage)).append(":").append(cfg_tag());

        const std::uint64_t key  = fnv1a(identifier);
        const std::uint64_t shash = fnv1a(source);

        if (const auto it = entries_.find(key);
            it != entries_.end() && it->second.source_hash == shash)
        {
            spdlog::debug("[shader] cache hit: {}", identifier);
            return { it->second.dxil.data(), it->second.dxil.size() };
        }

        //~ miss or stale -> compile
        spdlog::info("[shader] compiling: {}", identifier);
        std::vector<std::uint8_t> dxil;
        const shader_compile_desc desc{
            .source      = source,
            .entry_point = entry,
            .stage       = stage,
            .source_name = path,
        };
        if (!compiler_.compile(desc, dxil))
        {
            spdlog::error("[shader] compile failed: {}", identifier);
            return {};   //~ leaves any existing entry untouched
        }

        auto& slot = entries_[key];
        slot.key         = key;
        slot.source_hash = shash;
        slot.identifier  = identifier;
        slot.dxil        = std::move(dxil);
        dirty_ = true;

        return { slot.dxil.data(), slot.dxil.size() };
    }
} // namespace cots::graphics::shaders
