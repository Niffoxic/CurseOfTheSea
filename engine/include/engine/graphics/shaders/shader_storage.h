// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SHADER_STORAGE_H
#define CURSEOFTHESEA_SHADER_STORAGE_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace cots::graphics::shaders
{
    struct shader_cache_entry
    {
        std::uint64_t             key         { 0 };
        std::uint64_t             source_hash { 0 };
        std::string               identifier  {};
        std::vector<std::uint8_t> dxil        {};
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
    };
} // namespace cots::graphics::shaders

#endif

