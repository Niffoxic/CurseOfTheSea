// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GRAPHICS_EVENT_H
#define CURSEOFTHESEA_GRAPHICS_EVENT_H

#include <wrl/client.h>
#include <cstdint>
#include "engine/graphics/hardware/types.h"

namespace cots::events
{
    namespace device
    {
        struct creation_attempted{};

        // initialized with this adapter and its working
        // called after either creating for the 1st time
        // or device invalidated due to crash or selecting
        // different device runtime
        struct validated
        {
            std::uint32_t adapter_index;
        };

        // the reason for losing the device (mostly corrupt usages flags)
        // used for recreating device (recreation on release only)
        struct lost
        {
            HRESULT removal_reason;
        };
    } //~ namespace device

    namespace swapchain
    {
        struct will_recreate {};
        struct recreated
        {
            std::uint32_t width;
            std::uint32_t height;
            cots::graphics::hardware::display_mode mode;
        };

        struct resized
        {
            std::uint32_t width; std::uint32_t height;
        };

        struct occluded      {};
        struct restored      {};

        struct mode_changed
        {
             cots::graphics::hardware::display_mode new_mode;
        };

        struct set_display_mode
        {
            cots::graphics::hardware::display_mode mode;
        };
        struct set_windowed_size
        {
            std::uint32_t width;
            std::uint32_t height;
        };
    } // swapchain

    namespace shader
    {
        struct save  {};                     // flush cache to disk now
        struct reload{ std::uint64_t key; }; // recompile one
        struct clear {};                     // wipe cache force cold

        //~ notifications
        struct compiled
        {
            std::uint64_t key; bool from_cache; std::uint32_t bytes;
        };

        struct saved
        {
            std::uint32_t count;
        };

        struct failed
        {
            std::uint64_t key; //~ compile error
        };
    } // namespace shader

    namespace texture
    {
        //~ swap the test texture between live decode and baked
        struct toggle_bake_path {};

        //~ wipe the disk container for the test texture
        struct clear_bake_cache {};
    } // namespace texture

} // namespace cots::events::graphics

#endif //CURSEOFTHESEA_GRAPHICS_EVENT_H
