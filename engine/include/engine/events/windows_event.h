// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_WINDOWS_EVENT_H
#define CURSEOFTHESEA_WINDOWS_EVENT_H
#include <cstdint>

namespace cots::events
{
    struct window_resized
    {
        std::uint32_t width;
        std::uint32_t height;
    };

    struct window_minimized{};
    struct window_restored {};
}

#endif //CURSEOFTHESEA_WINDOWS_EVENT_H
