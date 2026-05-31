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
#include "trishul/renderer/shaders/shader_cache.h"

#include "trishul/core/engine_config.h"
#include "trishul/utils/logger.h"
#include "trishul/utils/statics.h"

namespace trishul::render::shaders
{
    namespace
    {
        //~ short tag for the stage folded into the cache key
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
    } //~ anonymous namespace

    shader_cache::~shader_cache() { deinitialize(); }

    bool shader_cache::initialize(std::unique_ptr<shader_storage> storage)
    {
        if (!compiler_.initialize()) return false;
        storage_ = std::move(storage);
        if (storage_ && !storage_->load_all(entries_))
            LOG_WARN("[shader] cache load failed starting cold");
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
        if (!statics::read_file(path, source))
        {
            LOG_ERROR("[shader] cannot read {}", path);
            return {};
        }

        //~ building the identity string everything that should bust the cache
        //~ goes in here path entry stage and the build tag
        std::string identifier;
        identifier.reserve(path.size() + 16);
        identifier.append(path).append(":").append(entry).append(":")
                  .append(stage_tag(stage)).append(":").append(statics::cfg_tag());

        const std::uint64_t key   = statics::fnv1a(identifier);
        const std::uint64_t shash = statics::fnv1a(source);

        //~ counting it a hit only when the source schema and depth pipeline all
        //~ still match a schema or depth bump forces a recompile even byte for
        //~ byte identical source
        if (const auto it = entries_.find(key);
            it != entries_.end()
            && it->second.source_hash    == shash
            && it->second.schema_version == k_cache_schema_version
            && it->second.depth_format   == config::DEPTH_FORMAT
            && it->second.depth_state    == config::DEPTH_STATE)
        {
            LOG_DEBUG("[shader] cache hit: {}", identifier);
            return {
                it->second.dxil.data(),
                it->second.dxil.size(),
                &it->second.input_layout,
                &it->second.bindings,
                &it->second.embedded_root_sig,
            };
        }

        LOG_INFO("[shader] compiling: {}", identifier);
        std::vector<std::uint8_t>         dxil;
        std::vector<vertex_input_element> layout;
        std::vector<reflected_binding>    bindings;
        std::vector<std::uint8_t>         embedded_rs;

        const shader_compile_desc desc
        {
            .source      = source,
            .entry_point = entry,
            .stage       = stage,
            .source_name = path,
        };

        //~ only vertex shaders bother reflecting an input layout
        const bool want_layout = (stage == shader_stage::vertex);
        if (!compiler_.compile(desc, dxil,
                               want_layout ? &layout : nullptr,
                               &bindings,
                               &embedded_rs))
        {
            LOG_ERROR("[shader] compile failed: {}", identifier);
            return {};   //~ leaving any existing entry untouched
        }

        //~ filling the slot with the fresh result and stamping the schema and
        //~ depth pipeline so the next run knows what it was baked against
        auto& slot = entries_[key];
        slot.key               = key;
        slot.schema_version    = k_cache_schema_version;
        slot.source_hash       = shash;
        slot.identifier        = identifier;
        slot.dxil              = std::move(dxil);
        slot.input_layout      = std::move(layout);
        slot.bindings          = std::move(bindings);
        slot.embedded_root_sig = std::move(embedded_rs);
        slot.depth_format      = config::DEPTH_FORMAT;
        slot.depth_state       = config::DEPTH_STATE;

        if (storage_ && !storage_->store_one(slot, entries_))
        {
            LOG_WARN("[shader] store_one failed for {}", identifier);
        }

        return
        {
            slot.dxil.data(),
            slot.dxil.size(),
            &slot.input_layout,
            &slot.bindings,
            &slot.embedded_root_sig,
        };
    }

    void shader_cache::clear()
    {
        entries_.clear();
        if (storage_)
            (void)storage_->store_all(entries_);   //~ writing out an empty archive
        LOG_INFO("[shader] cache cleared");
    }

    bool shader_cache::recompile(const std::uint64_t key)
    {
        if (key == 0)
        {
            //~ zeroing every source hash so the whole lot rebuilds next time
            for (auto& [k, e] : entries_)
                e.source_hash = 0;
            LOG_INFO("[shader] all shaders marked for recompile");
            return true;
        }
        if (const auto it = entries_.find(key); it != entries_.end())
        {
            //~ this one rebuilds on its next get_or_compile
            it->second.source_hash = 0;
            return true;
        }
        return false;
    }
} // namespace trishul::render::shaders