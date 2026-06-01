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
#ifndef CURSEOFTHESEA_CLEAR_COLOR_H
#define CURSEOFTHESEA_CLEAR_COLOR_H

namespace trishul::render
{
    //~ crimson blood red 8a0a03 the one clear color engine wide never changes so
    //~ targets bake it as their optimized clear value and the driver fast clears
    //~ change it here and every clear plus every optimized clear value follows
    inline constexpr float k_clear_color[4]
    {
        138.0f / 255.0f, //~ 0.5411
         10.0f / 255.0f, //~ 0.0392
          3.0f / 255.0f, //~ 0.0117
          1.0f
    };
} // namespace trishul::render

#endif //CURSEOFTHESEA_CLEAR_COLOR_H
