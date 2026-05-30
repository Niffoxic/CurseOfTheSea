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
#ifndef CURSEOFTHESEA_WINDOW_EVENT_H
#define CURSEOFTHESEA_WINDOW_EVENT_H
#include <cstdint>

namespace trishul::events
{
    //~ client area changed includes resolution changes and fullscreen toggles
    struct window_resized
    {
        std::uint32_t width;
        std::uint32_t height;
    };

    struct window_minimized {};
    struct window_restored  {};

    //~ borderless fullscreen toggled
    struct window_fullscreen_changed
    {
        bool fullscreen;
    };

    //~ focus gained or lost
    struct window_focus_changed
    {
        bool focused;
    };

    //~ top left moved in screen space
    struct window_moved
    {
        std::int32_t x;
        std::int32_t y;
    };

    //~ user asked to close
    struct window_closed {};
} // namespace trishul::events

#endif //CURSEOFTHESEA_WINDOW_EVENT_H
