// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_TEXTURE_CACHE_H
#define CURSEOFTHESEA_TEXTURE_CACHE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "engine/graphics/textures/baked_storage.h"
#include "engine/graphics/textures/texture_baker.h"
#include "engine/graphics/textures/texture_intent.h"

namespace cots::graphics::textures
{
    //~ result of a bake or load
    struct baked_blob
    {
        std::vector<std::uint8_t> dds;
        std::uint64_t             source_hash { 0 };
        texture_intent            intent      { texture_intent::albedo };

        [[nodiscard]] bool valid() const noexcept { return !dds.empty(); }
    };

    //~ front door for baked textures
    class texture_cache final
    {
    public:
         texture_cache() = default;
        ~texture_cache() = default;

        texture_cache           (const texture_cache&) = delete;
        texture_cache& operator=(const texture_cache&) = delete;

        [[nodiscard]] bool initialize  ();
                      void deinitialize() noexcept;

        //~ load or bake on miss
        [[nodiscard]] bool get_or_bake(std::string_view source_path,
                                       texture_intent intent,
                                       baked_blob& out);

        //~ wipe the disk container
        void invalidate(std::string_view source_path) const;

    private:
        [[nodiscard]] static std::string baked_path_for(std::string_view source_path);

        [[nodiscard]] bool try_load_cached(const std::string& baked_path,
                                           std::uint64_t      expected_hash,
                                           texture_intent     expected_intent,
                                           baked_blob&        out) const;

        [[nodiscard]] bool write_container(const std::string& baked_path,
                                           std::uint64_t      source_hash,
                                           texture_intent     intent,
                                           const std::vector<std::uint8_t>& dds) const;

    private:
        texture_baker baker_;
    };
} // namespace cots::graphics::textures

#endif //CURSEOFTHESEA_TEXTURE_CACHE_H
