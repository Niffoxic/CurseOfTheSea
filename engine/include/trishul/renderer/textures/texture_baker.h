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
#ifndef CURSEOFTHESEA_TEXTURE_BAKER_H
#define CURSEOFTHESEA_TEXTURE_BAKER_H

#include <cstdint>
#include <string_view>
#include <vector>

#include "trishul/renderer/textures/texture_intent.h"

namespace trishul::render::textures
{
    //~ knobs the editor can flip per bake everything defaults to the fast
    //~ sensible path so a plain bake(path, intent, out) keeps working untouched
    struct bake_options
    {
        bool generate_mips { true  }; //~ off bakes just the base level
        bool high_quality  { false }; //~ on takes the slow bc7 path for finals
    };

    //~ decoding mip generating and bc compressing a source image into a dds blob
    //~ the heavy lifting behind the texture cache the editor usually goes through
    //~ the cache not this directly
    class texture_baker final
    {
    public:
         texture_baker() = default;
        ~texture_baker() = default;

        texture_baker           (const texture_baker&) = delete;
        texture_baker& operator=(const texture_baker&) = delete;

        [[nodiscard]] bool initialize  ();
                      void deinitialize() noexcept;

        //~ baking a source file down to a dds blob the intent picks the bc
        //~ format opts tweaks quality and mips and defaults to fast with mips
        [[nodiscard]] bool bake(std::string_view source_path,
                                texture_intent intent,
                                std::vector<std::uint8_t>& out_dds,
                                const bake_options& opts = {});
    };
} // namespace trishul::render::textures

#endif //CURSEOFTHESEA_TEXTURE_BAKER_H
