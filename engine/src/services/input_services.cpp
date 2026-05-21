// Created by Niffoxic (Harsh Dubey)
#include "engine/system/service_registry.h"
#include "engine/system/feature_locator.h"
#include "engine/platform/platform_windows.h"

namespace
{
    auto win() { return cots::feature::locator::resolve<cots::platform::windows>(); }

    //~ keyboard
    bool key_down(const int vk) { return win()->keyboard.is_down(vk); }
    bool key_up  (const int vk) { return win()->keyboard.is_up  (vk); }

    bool key_pressed (const int vk) { return win()->keyboard.pressed (vk); }
    bool key_released(const int vk) { return win()->keyboard.released(vk); }

    bool keys_all_down(const int* vks, const int count)
    {
        if (!vks || count <= 0) return false;

        const auto& kb = win()->keyboard;
        for (int i = 0; i < count; ++i)
        {
            if (!kb.is_down(vks[i])) return false;
        }
        return true;
    }

    bool keys_any_down(const int* vks, const int count)
    {
        if (!vks || count <= 0) return false;

        const auto& kb = win()->keyboard;
        for (int i = 0; i < count; ++i)
        {
            if (kb.is_down(vks[i])) return true;
        }
        return false;
    }

    bool any_key_pressed() { return win()->keyboard.any_pressed(); }

    //~ keyboard - modifiers
    bool ctrl_down () { return win()->keyboard.ctrl_down (); }
    bool shift_down() { return win()->keyboard.shift_down(); }
    bool alt_down  () { return win()->keyboard.alt_down  (); }
    bool super_down() { return win()->keyboard.super_down(); }

    //~ chord
    bool chord(const int vk, const std::uint8_t mods, const bool strict)
    {
        return win()->keyboard.chord(
            vk,
            static_cast<cots::platform::key_mode>(mods),
            strict);
    }

    //~ mouse - buttons
    bool mouse_down(const int button)
    {
        return win()->mouse.is_down(static_cast<cots::platform::mouse_button>(button));
    }
    bool mouse_pressed(const int button)
    {
        return win()->mouse.pressed(static_cast<cots::platform::mouse_button>(button));
    }
    bool mouse_released(const int button)
    {
        return win()->mouse.released(static_cast<cots::platform::mouse_button>(button));
    }

    void mouse_position(int* x, int* y)
    {
        const auto p = win()->mouse.position();
        if (x) *x = p.x;
        if (y) *y = p.y;
    }

    void mouse_raw_delta(int* x, int* y)
    {
        const auto d = win()->mouse.raw_delta();
        if (x) *x = d.x;
        if (y) *y = d.y;
    }

    float mouse_wheel() { return win()->mouse.wheel(); }

    void install(cots::module::services& s)
    {
        //~ keyboard
        s.input.key_down        = &key_down;
        s.input.key_up          = &key_up;
        s.input.key_pressed     = &key_pressed;
        s.input.key_released    = &key_released;
        s.input.keys_all_down   = &keys_all_down;
        s.input.keys_any_down   = &keys_any_down;
        s.input.any_key_pressed = &any_key_pressed;
        s.input.ctrl_down       = &ctrl_down;
        s.input.shift_down      = &shift_down;
        s.input.alt_down        = &alt_down;
        s.input.super_down      = &super_down;
        s.input.chord           = &chord;

        //~ mouse
        s.input.mouse_down      = &mouse_down;
        s.input.mouse_pressed   = &mouse_pressed;
        s.input.mouse_released  = &mouse_released;
        s.input.mouse_position  = &mouse_position;
        s.input.mouse_raw_delta = &mouse_raw_delta;
        s.input.mouse_wheel     = &mouse_wheel;
    }
}

COTS_INSTALL_SERVICES(install)
