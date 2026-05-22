// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_VERTEX_LAYOUT_H
#define CURSEOFTHESEA_VERTEX_LAYOUT_H

#include <cstdint>
#include <string>

namespace cots::graphics::shaders
{
    // reflected vertex-shader input attribute
    struct vertex_input_element
    {
        std::string   semantic_name {};
        std::uint32_t semantic_index{ 0 };
        std::uint32_t format        { 0 };
        std::uint32_t input_slot    { 0 };
    };
} // namespace cots::graphics::shaders

#endif //CURSEOFTHESEA_VERTEX_LAYOUT_H
