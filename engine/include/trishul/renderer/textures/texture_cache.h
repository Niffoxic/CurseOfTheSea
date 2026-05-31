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
#ifndef CURSEOFTHESEA_TEXTURE_CACHE_H
#define CURSEOFTHESEA_TEXTURE_CACHE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "trishul/renderer/textures/baked_storage.h"
#include "trishul/renderer/textures/texture_baker.h"
#include "trishul/renderer/textures/texture_intent.h"

namespace trishul::render::textures
{
    //~ what comes back from a load or a bake the dds plus the bookkeeping the
    //~ cache uses to decide if it is still fresh
    struct baked_blob
    {
        std::vector<std::uint8_t> dds;
        std::uint64_t             source_hash { 0 };
        texture_intent            intent      { texture_intent::albedo };

        [[nodiscard]] bool valid() const noexcept { return !dds.empty(); }
    };

    //~ the front door for baked textures the editor asks for a source and an
    //~ intent and gets a ready dds back loading the cached one or baking on a miss
    class texture_cache final
    {
    public:
         texture_cache() = default;
        ~texture_cache() = default;

        texture_cache           (const texture_cache&) = delete;
        texture_cache& operator=(const texture_cache&) = delete;

        [[nodiscard]] bool initialize  ();
                      void deinitialize() noexcept;

        //~ loading the baked container or baking it on a miss opts forwards to
        //~ the baker so the editor can ask for a high quality rebake
        [[nodiscard]] bool get_or_bake(std::string_view source_path,
                                       texture_intent intent,
                                       baked_blob& out,
                                       const bake_options& opts = {});

        //~ wiping the baked container off disk so the next get_or_bake rebakes
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
} // namespace trishul::render::textures

#endif //CURSEOFTHESEA_TEXTURE_CACHE_H
