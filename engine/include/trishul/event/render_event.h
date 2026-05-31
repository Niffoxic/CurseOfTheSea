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
#ifndef CURSEOFTHESEA_DEVICE_EVENT_H
#define CURSEOFTHESEA_DEVICE_EVENT_H

#include <cstdint>
#include <winerror.h>
#include "trishul/renderer/hardware/types.h"

namespace trishul::events
{
    //~ first successful bring up
    struct device_initialized
    {
        std::uint32_t adapter_index{};
    };

    struct device_recreating{}; //~ called before device is being recreated

    struct device_recreated //~ called just after device is recreated
    {
        std::uint32_t adapter_index{};
    };

    //~ recreate could not bring the device back treating as fatal
    struct device_recreate_failed{};

    struct device_lost //~ GG you crashed it AGAIN!!
    {
        HRESULT removal_reason{};
    };

    //~ monitor changed after a rescan
    struct outputs_changed
    {
        std::uint32_t count{};
    };

    namespace swapchain
    {
        struct will_recreate {};
        struct recreated
        {
            std::uint32_t width;
            std::uint32_t height;
            render::hardware::display_mode mode;
        };

        struct resized
        {
            std::uint32_t width; std::uint32_t height;
        };

        struct occluded      {};
        struct restored      {};

        struct mode_changed
        {
            render::hardware::display_mode new_mode;
        };
    } // swapchain
} // namespace trishul::events

#endif //CURSEOFTHESEA_DEVICE_EVENT_H
