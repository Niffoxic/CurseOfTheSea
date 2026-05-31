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
#ifndef CURSEOFTHESEA_SHADER_STORAGE_H
#define CURSEOFTHESEA_SHADER_STORAGE_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "trishul/renderer/shaders/vertex_layout.h"
#include "trishul/renderer/shaders/shader_compiler.h"

namespace trishul::render::shaders
{
    //~ bumping this on any change to the entry layout below forces a cold
    //~ recompile so a stale cache can never feed garbage into a pso
    constexpr std::uint32_t k_cache_schema_version = 3;

    //~ everything we keep per shader the depth fields pin which depth pipeline
    //~ this got baked for engine wide DEPTH_FORMAT and DEPTH_STATE live in
    //~ engine_config so the dsv and the pso can never quietly drift apart
    struct shader_cache_entry
    {
        std::uint64_t             key           { 0 };
        std::uint32_t             schema_version{ 0 };
        std::uint64_t             source_hash   { 0 };
        std::string               identifier          {};
        std::vector<std::uint8_t> dxil                {};
        std::vector<vertex_input_element> input_layout{};

        std::uint32_t depth_format{ 0 };
        std::uint32_t depth_state { 0 };

        //~ resources the bytecode binds reflected at compile time
        std::vector<reflected_binding> bindings;

        //~ the shaders own root signature blob if it declared one
        std::vector<std::uint8_t>      embedded_root_sig;
    };

    using cache_map = std::unordered_map<std::uint64_t, shader_cache_entry>;

    //~ the persistence policy a swappable strategy so the cache does not care
    //~ whether entries land in a binary blob a json file or whatever comes next
    class shader_storage
    {
    public:
        virtual ~shader_storage() = default;

        //~ cold loading the whole archive and rewriting all of it
        [[nodiscard]] virtual bool load_all (cache_map& out)       = 0;
        [[nodiscard]] virtual bool store_all(const cache_map& in)  = 0;

        //~ durably stashing one entry the moment it compiles so a crash mid run
        //~ does not throw away the work
        [[nodiscard]] virtual bool store_one(const shader_cache_entry& entry,
                                             const cache_map& full) = 0;
    };
} // namespace trishul::render::shaders

#endif //CURSEOFTHESEA_SHADER_STORAGE_H