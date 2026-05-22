// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SHADER_STORAGE_H
#define CURSEOFTHESEA_SHADER_STORAGE_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "engine/graphics/shaders/vertex_layout.h"

namespace cots::graphics::shaders
{
    constexpr std::uint32_t k_cache_schema_version = 2;

    //~ engine-wide depth pipeline constants these get recorded in every cache
    constexpr std::uint32_t k_engine_depth_format = 20u;

    //~ packed bits 0 = depth enable, 1 = depth write,
    //  bits 4..7 = D3D12 comparison func
    //  current values are depth on, write, GREATER = 5 for reversed z
    constexpr std::uint32_t k_engine_depth_state =
        (1u << 0) | (1u << 1) | (5u << 4);

    //~ depth pipeline info recorded per entry so
    //  dsv format and pso agree to each other
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
    };

    using cache_map = std::unordered_map<std::uint64_t, shader_cache_entry>;

    //~ swappable persistence policy
    class shader_storage
    {
    public:
        virtual ~shader_storage() = default;

        //~ cold cache
        [[nodiscard]] virtual bool load_all (cache_map& out)       = 0;
        [[nodiscard]] virtual bool store_all(const cache_map& in)  = 0;

        //~ durably persist a single entry right after compile
        [[nodiscard]] virtual bool store_one(const shader_cache_entry& entry,
                                             const cache_map& full) = 0;
    };
} // namespace cots::graphics::shaders

#endif

