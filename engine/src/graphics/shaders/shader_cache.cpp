// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/shaders/shader_cache.h"

#include <cots/cots_config.h>
#include <spdlog/spdlog.h>

#include "engine/utils/helpers.h"

namespace cots::graphics::shaders
{
    namespace
    {
        const char* stage_tag(const shader_stage s) noexcept
        {
            switch (s)
            {
                case shader_stage::vertex:  return "vs";
                case shader_stage::pixel:   return "ps";
                case shader_stage::compute: return "cs";
                default:                    return "xx";
            }
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

    void shader_cache::flush() const
    {
        if (storage_)
        {
            (void)storage_->store_all(entries_);
        }
    }

    shader_bytecode shader_cache::get_or_compile(
        std::string_view path,
        const std::string_view entry,
        const shader_stage stage)
    {
        std::string source;
        if (!helpers::read_file(path, source))
        {
            spdlog::error("[shader] cannot read {}", path);
            return {};
        }

        //~ slot identifier hash
        std::string identifier;
        identifier.reserve(path.size() + 16);
        identifier.append(path).append(":").append(entry).append(":")
                  .append(stage_tag(stage)).append(":").append(helpers::cfg_tag());

        const std::uint64_t key   = helpers::fnv1a(identifier);
        const std::uint64_t shash = helpers::fnv1a(source);

        if (const auto it = entries_.find(key);
            it != entries_.end() && it->second.source_hash == shash)
        {
            spdlog::debug("[shader] cache hit: {}", identifier);
            return { it->second.dxil.data(), it->second.dxil.size() };
        }

        //~ miss or stale then compile
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

        if (storage_ && !storage_->store_one(slot, entries_))
            spdlog::warn("[shader] store_one failed for {}", identifier);

        return { slot.dxil.data(), slot.dxil.size() };
    }

    void shader_cache::clear()
    {
        entries_.clear();
        if (storage_) (void)storage_->store_all(entries_);   //~ writes empty archive
        spdlog::info("[shader] cache cleared");
    }

    bool shader_cache::recompile(const std::uint64_t key)
    {
        if (key == 0)
        {
            //~ force-stale everything
            for (auto& [k, e] : entries_) e.source_hash = 0;
            spdlog::info("[shader] all shaders marked for recompile");
            return true;
        }
        if (const auto it = entries_.find(key); it != entries_.end())
        {
            it->second.source_hash = 0;   //~ there will be a rebuild on next get
            return true;
        }
        return false;
    }
} // namespace cots::graphics::shaders
