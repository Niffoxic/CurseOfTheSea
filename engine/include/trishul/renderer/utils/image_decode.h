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
#ifndef CURSEOFTHESEA_IMAGE_DECODE_H
#define CURSEOFTHESEA_IMAGE_DECODE_H

#include <cstdint>
#include <string_view>
#include <vector>

namespace trishul::render::utils
{
    //~ owned rgba8 pixels tightly packed width times height times four
    struct decoded_image
    {
        std::vector<std::uint8_t> pixels;
        std::uint32_t             width  { 0 };
        std::uint32_t             height { 0 };

        [[nodiscard]] bool valid() const noexcept
        {
            return width > 0 && height > 0 &&
                   pixels.size() == static_cast<std::size_t>(width) * height * 4u;
        }

        [[nodiscard]] std::uint32_t row_pitch() const noexcept
        {
            return width * 4u;
        }
    };

    //~ decoding a png or jpeg off disk into rgba8 returns false if it cannot
    [[nodiscard]] bool decode_image_file(std::string_view path, decoded_image& out);

    //~ a procedural checkerboard the go to stand in when a texture is missing
    //~ colours are packed abgr cell is the square size in pixels the editor can
    //~ dial all of these to taste
    void make_checkerboard(std::uint32_t width, std::uint32_t height,
                           std::uint32_t cell,
                           std::uint32_t color_a, //~ packed alpha blue green red
                           std::uint32_t color_b,
                           decoded_image& out);
} // namespace trishul::render::utils

#endif //CURSEOFTHESEA_IMAGE_DECODE_H
