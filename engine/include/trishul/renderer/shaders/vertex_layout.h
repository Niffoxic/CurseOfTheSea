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
#ifndef CURSEOFTHESEA_VERTEX_LAYOUT_H
#define CURSEOFTHESEA_VERTEX_LAYOUT_H

#include <cstdint>
#include <string>

namespace trishul::render::shaders
{
    //~ one vertex input attribute the compiler reflects these straight off the
    //~ vertex shader signature so the pso input layout always matches the dxil
    struct vertex_input_element
    {
        std::string   semantic_name {};
        std::uint32_t semantic_index{ 0 };
        std::uint32_t format        { 0 };
        std::uint32_t input_slot    { 0 };
    };
} // namespace trishul::render::shaders

#endif //CURSEOFTHESEA_VERTEX_LAYOUT_H