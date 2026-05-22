// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_TEXTURE_BAKER_H
#define CURSEOFTHESEA_TEXTURE_BAKER_H

#include <cstdint>
#include <string_view>
#include <vector>

#include "engine/graphics/textures/texture_intent.h"

namespace cots::graphics::textures
{
    //~ decode mipgen and compress
    //~ produces a dds blob
    class texture_baker final
    {
    public:
         texture_baker() = default;
        ~texture_baker() = default;

        texture_baker           (const texture_baker&) = delete;
        texture_baker& operator=(const texture_baker&) = delete;

        [[nodiscard]] bool initialize  ();
                      void deinitialize() noexcept;

        //~ bake source to dds
        [[nodiscard]] bool bake(std::string_view source_path,
                                texture_intent intent,
                                std::vector<std::uint8_t>& out_dds);
    };
} // namespace cots::graphics::textures

#endif //CURSEOFTHESEA_TEXTURE_BAKER_H
