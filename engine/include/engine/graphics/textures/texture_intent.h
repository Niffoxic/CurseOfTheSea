// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_TEXTURE_INTENT_H
#define CURSEOFTHESEA_TEXTURE_INTENT_H

#include <cstdint>

namespace cots::graphics::textures
{
    //~ what the texture is for
    //~ drives the bc format
    enum class texture_intent : std::uint8_t
    {
        albedo  = 0,   //~ bc seven srgb color
        normal  = 1,   //~ bc five tangent space
        mask    = 2,   //~ bc four single channel
        hdr     = 3,   //~ bc six h float
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
} // namespace cots::graphics::textures

#endif //CURSEOFTHESEA_TEXTURE_INTENT_H
