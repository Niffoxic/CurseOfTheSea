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
#include <trishul/utils/timer.h>
#include <trishul/event/dispatcher.h>
#include <trishul/event/window_event.h>
#include <trishul/renderer/render.h>

#include <format>
#include <print>
#include <string>
#include <windows.h>

namespace trishul
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

        //~ gpu switch test asking the renderer to move to a specific adapter
        void switch_to_adapter(const std::uint32_t index)
        {
            auto* gfx = trishul::service_locator::try_get<trishul::render::graphics>();
            if (!gfx) { std::println("renderer service missing"); return; }

            trishul::render::display_settings s = gfx->current_display_settings();
            s.manual_adapter = true;
            s.adapter_index  = index;

            std::println("requesting switch to adapter {}", index);
            gfx->request_display_settings(s);
        }

        //~ dump what the menu would show gpus and monitors
        void list_display_options()
        {
            auto* gfx = trishul::service_locator::try_get<trishul::render::graphics>();
            if (!gfx) { std::println("renderer service missing"); return; }

            const trishul::render::display_capabilities caps = gfx->display_options();
            std::println("{} adapter(s) current index {}",
                caps.adapters.size(), caps.current_adapter_index);
            for (const auto& a : caps.adapters)
            {
                std::println("   [{}] {} ({}) {} MB{}",
                    a.index, a.name, trishul::render::vendor_name(a.vendor_id),
                    a.dedicated_video_memory / (1024ull * 1024ull),
                    a.is_warp ? " WARP" : "");
            }

            std::println("{} output(s)", caps.outputs.size());
            for (const auto& o : caps.outputs)
            {
                std::println("   [{}] {} {}x{}{} {} mode(s)",
                    o.index, o.name, o.desktop_width, o.desktop_height,
                    o.is_primary ? " primary" : "", o.modes.size());
            }
        }
    } // namespace anonymous

    void game_layer::on_attach()
    {
        std::println("input test ready");
        std::println("  F1 toggle fullscreen  F2 1280x720  F3 1600x900  F4 1920x1080");
        std::println("  F5 list gpus + monitors   keys 0 1 2 switch gpu adapter");

        if (auto* timers = trishul::service_locator::try_get<trishul::timer_manager>())
        {
            timers->set_timer([]{ std::println("[timer] five seconds elapsed"); },
                              5.0f, true);
        }

        //~ subscribe to window events fired by the platform
        dispatcher_ = trishul::service_locator::try_get<trishul::events::dispatcher>();
        if (dispatcher_)
        {
            using namespace trishul::events;
            dispatcher_->subscribe<window_resized,            &game_layer::on_window_resized>   (*this);
            dispatcher_->subscribe<window_fullscreen_changed, &game_layer::on_window_fullscreen>(*this);
            dispatcher_->subscribe<window_focus_changed,      &game_layer::on_window_focus>     (*this);
            dispatcher_->subscribe<window_minimized,          &game_layer::on_window_minimized> (*this);
            dispatcher_->subscribe<window_restored,           &game_layer::on_window_restored>  (*this);
            dispatcher_->subscribe<window_closed,             &game_layer::on_window_closed>    (*this);

            //~ gpu switch result lands here device runs the recreate async
            dispatcher_->subscribe<device_recreated,       &game_layer::on_device_recreated>      (*this);
            dispatcher_->subscribe<device_recreate_failed, &game_layer::on_device_recreate_failed>(*this);
        }
    }

    void game_layer::on_detach()
    {
        if (dispatcher_)
        {
            using namespace trishul::events;
            dispatcher_->unsubscribe<window_resized,            &game_layer::on_window_resized>   (*this);
            dispatcher_->unsubscribe<window_fullscreen_changed, &game_layer::on_window_fullscreen>(*this);
            dispatcher_->unsubscribe<window_focus_changed,      &game_layer::on_window_focus>     (*this);
            dispatcher_->unsubscribe<window_minimized,          &game_layer::on_window_minimized> (*this);
            dispatcher_->unsubscribe<window_restored,           &game_layer::on_window_restored>  (*this);
            dispatcher_->unsubscribe<window_closed,             &game_layer::on_window_closed>    (*this);

            dispatcher_->unsubscribe<device_recreated,       &game_layer::on_device_recreated>      (*this);
            dispatcher_->unsubscribe<device_recreate_failed, &game_layer::on_device_recreate_failed>(*this);
            dispatcher_ = nullptr;
        }
    }

    //~ window event handlers
    void game_layer::on_window_resized(const trishul::events::window_resized& e)
    {
        std::println("[event] resized {}x{}", e.width, e.height);
    }
    void game_layer::on_window_fullscreen(const trishul::events::window_fullscreen_changed& e)
    {
        std::println("[event] fullscreen {}", e.fullscreen ? "on" : "off");
    }
    void game_layer::on_window_focus(const trishul::events::window_focus_changed& e)
    {
        std::println("[event] focus {}", e.focused ? "gained" : "lost");
    }
    void game_layer::on_window_minimized(const trishul::events::window_minimized&)
    {
        std::println("[event] minimized");
    }
    void game_layer::on_window_restored(const trishul::events::window_restored&)
    {
        std::println("[event] restored");
    }
    void game_layer::on_window_closed(const trishul::events::window_closed&)
    {
        std::println("[event] closed");
    }

    //~ gpu switch results arrived
    void game_layer::on_device_recreated(const trishul::events::device_recreated& e)
    {
        std::println("device recreated on adapter {}", e.adapter_index);
    }
    void game_layer::on_device_recreate_failed(const trishul::events::device_recreate_failed&)
    {
        std::println("device recreate FAILED device is down check the logs");
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

        if (kb.pressed(VK_F1)) window->toggle_fullscreen();
        if (kb.pressed(VK_F2)) window->set_resolution(1280, 720);
        if (kb.pressed(VK_F3)) window->set_resolution(1600, 900);
        if (kb.pressed(VK_F4)) window->set_resolution(1920, 1080);

        //~ gpu adapter switching tests
        if (kb.pressed(VK_F5)) list_display_options();
        if (kb.pressed('0'))   switch_to_adapter(0);
        if (kb.pressed('1'))   switch_to_adapter(1);
        if (kb.pressed('2'))   switch_to_adapter(2);

        std::string mods;
        if (kb.ctrl_down ()) mods += "ctrl+";
        if (kb.shift_down()) mods += "shift+";
        if (kb.alt_down  ()) mods += "alt+";

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
