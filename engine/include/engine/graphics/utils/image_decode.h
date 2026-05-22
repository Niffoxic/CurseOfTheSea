// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_IMAGE_DECODE_H
#define CURSEOFTHESEA_IMAGE_DECODE_H

#include <cstdint>
#include <string_view>
#include <vector>

namespace cots::graphics::utils
{
    //~ owned rgba pixels
    struct decoded_image
    {
        std::vector<std::uint8_t> pixels;
        std::uint32_t             width  { 0 };
        std::uint32_t             height { 0 };

        [[nodiscard]] bool valid() const noexcept
        {
            return width > 0 && height > 0 && pixels.size() == static_cast<std::size_t>(width) * height * 4u;
        }

        [[nodiscard]] std::uint32_t row_pitch() const noexcept
        {
            return width * 4u;
        }
    };

    //~ decode a png or jpeg
    [[nodiscard]] bool decode_image_file(std::string_view path, decoded_image& out);

    //~ procedural checkerboard fallback
    void make_checkerboard(std::uint32_t width, std::uint32_t height,
                           std::uint32_t cell,
                           std::uint32_t color_a, //~ packed alpha blue green red
                           std::uint32_t color_b,
                           decoded_image& out);
} // namespace cots::graphics::utils

#endif //CURSEOFTHESEA_IMAGE_DECODE_H
