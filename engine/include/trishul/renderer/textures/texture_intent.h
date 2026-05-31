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
#ifndef CURSEOFTHESEA_TEXTURE_INTENT_H
#define CURSEOFTHESEA_TEXTURE_INTENT_H

#include <cstdint>

namespace trishul::render::textures
{
    //~ what a texture is for the editor picks this per texture and it decides
    //~ which bc format the baker compresses to so colour stays srgb and normals
    //~ keep their precision
    enum class texture_intent : std::uint8_t
    {
        albedo  = 0,   //~ bc7 srgb colour
        normal  = 1,   //~ bc5 tangent space
        mask    = 2,   //~ bc4 single channel
        hdr     = 3,   //~ bc6h float
    };

    [[nodiscard]] inline const char* to_string(const texture_intent i) noexcept
    {
        switch (i)
        {
        case texture_intent::albedo: return "albedo";
        case texture_intent::normal: return "normal";
        case texture_intent::mask:   return "mask";
        case texture_intent::hdr:    return "hdr";
        }
        return "?";
    }
} // namespace trishul::render::textures

#endif //CURSEOFTHESEA_TEXTURE_INTENT_H
