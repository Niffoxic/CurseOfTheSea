// Created by Niffoxic (Harsh Dubey)
#include "engine/graphics/utils/image_decode.h"

#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO_DEPRECATIONS
#include <stb_image.h>

#include <cstring>
#include <string>

namespace cots::graphics::utils
{
    bool decode_image_file(const std::string_view path, decoded_image& out)
    {
        out = decoded_image{};

        const std::string spath(path);
        int w = 0;
        int h = 0;
        int channels_in_file = 0;

        stbi_uc* pixels = stbi_load(
            spath.c_str(), &w, &h, &channels_in_file, STBI_rgb_alpha);

        if (!pixels)
        {
            spdlog::error("[image] decode failed for '{}' reason '{}'",
                          spath, stbi_failure_reason() ? stbi_failure_reason() : "unknown");
            return false;
        }

        out.width  = static_cast<std::uint32_t>(w);
        out.height = static_cast<std::uint32_t>(h);

        const std::size_t bytes = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u;
        out.pixels.assign(pixels, pixels + bytes);
        stbi_image_free(pixels);

        spdlog::info("[image] decoded '{}' {}x{}", spath, out.width, out.height);
        return true;
    }

    void make_checkerboard(const std::uint32_t width, const std::uint32_t height,
                           const std::uint32_t cell,
                           const std::uint32_t color_a,
                           const std::uint32_t color_b,
                           decoded_image& out)
    {
        out = decoded_image{};
        out.width  = width  > 0 ? width  : 1;
        out.height = height > 0 ? height : 1;

        const std::size_t total = static_cast<std::size_t>(out.width) * out.height * 4u;
        out.pixels.resize(total);

        const std::uint32_t step = cell > 0 ? cell : 1u;
        for (std::uint32_t y = 0; y < out.height; ++y)
        {
            for (std::uint32_t x = 0; x < out.width; ++x)
            {
                const bool on  = ((x / step) ^ (y / step)) & 1u;
                const std::uint32_t c = on ? color_b : color_a;

                std::uint8_t* dst = &out.pixels[(static_cast<std::size_t>(y) * out.width + x) * 4u];
                dst[0] = static_cast<std::uint8_t>((c >>  0) & 0xFFu); //~ red
                dst[1] = static_cast<std::uint8_t>((c >>  8) & 0xFFu); //~ green
                dst[2] = static_cast<std::uint8_t>((c >> 16) & 0xFFu); //~ blue
                dst[3] = static_cast<std::uint8_t>((c >> 24) & 0xFFu); //~ alpha
            }
        }
    }
} // namespace cots::graphics::utils
