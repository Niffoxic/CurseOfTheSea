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
#include "game_layer.h"

#include <trishul/core/service_locator.h>
#include <trishul/platform/platform_windows.h>
#include <trishul/platform/inputs/keyboard_component.h>
#include <trishul/platform/inputs/mouse_component.h>

#include <format>
#include <print>
#include <string>
#include <windows.h>

namespace cots
{
    namespace
    {
        bool is_modifier(const int vk) noexcept
        {
            switch (vk)
            {
            case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
            case VK_SHIFT:   case VK_LSHIFT:   case VK_RSHIFT:
            case VK_MENU:    case VK_LMENU:    case VK_RMENU:
            case VK_LWIN:    case VK_RWIN:
                return true;
            default:
                return false;
            }
        }

        std::string key_label(const int vk)
        {
            if (vk >= 'A' && vk <= 'Z') return std::string(1, static_cast<char>(vk));
            if (vk >= '0' && vk <= '9') return std::string(1, static_cast<char>(vk));

            switch (vk)
            {
            case VK_SPACE:  return "space";
            case VK_RETURN: return "enter";
            case VK_ESCAPE: return "esc";
            case VK_TAB:    return "tab";
            case VK_BACK:   return "backspace";
            case VK_LEFT:   return "left";
            case VK_RIGHT:  return "right";
            case VK_UP:     return "up";
            case VK_DOWN:   return "down";
            default:        return std::format("vk 0x{:02X}", vk);
            }
        }
    } // namespace anonymous

    void game_layer::on_attach()
    {
        std::println("input test ready press keys or click");
    }

    void game_layer::on_detach()
    {
    }

    void game_layer::on_update(float dt)
    {
        using namespace trishul;
        using mb = inputs::mouse_button;
        (void)dt;

        auto* window = service_locator::try_get<platform_window>();
        if (!window) return;

        const auto& kb = window->get_component<keyboard>();
        const auto& ms = window->get_component<mouse>();

        //~ modifier prefix for non modifier keys
        std::string mods;
        if (kb.ctrl_down ()) mods += "ctrl+";
        if (kb.shift_down()) mods += "shift+";
        if (kb.alt_down  ()) mods += "alt+";

        //~ which key this frame
        for (int vk = 0; vk < 256; ++vk)
        {
            if (!kb.pressed(vk)) continue;

            if (is_modifier(vk)) std::println("[key] {}",   key_label(vk));
            else                 std::println("[key] {}{}", mods, key_label(vk));
        }

        //~ which mouse action this frame
        const auto pos = ms.position();

        if (ms.pressed(mb::left))   std::println("[mouse] left click at {},{}",  pos.x, pos.y);
        if (ms.pressed(mb::right))  std::println("[mouse] right click at {},{}", pos.x, pos.y);
        if (ms.pressed(mb::middle)) std::println("[mouse] middle click");
        if (ms.pressed(mb::x1))     std::println("[mouse] x1 back");
        if (ms.pressed(mb::x2))     std::println("[mouse] x2 forward");

        if (ms.double_clicked(mb::left))  std::println("[mouse] left double click");
        if (ms.double_clicked(mb::right)) std::println("[mouse] right double click");

        if (ms.wheel() != 0.0f)
            std::println("[mouse] wheel {:.1f}", ms.wheel());
    }
} // namespace cots
