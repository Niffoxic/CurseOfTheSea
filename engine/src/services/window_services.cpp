// Created by Niffoxic (Harsh Dubey)
#include "engine/system/service_registry.h"
#include "engine/system/define_features.h"
#include "engine/platform/platform_windows.h"

namespace
{
    auto win()
    {
        return cots::feature::locator::resolve<cots::platform::windows>();
    }

    void get_size(int* w, int* h)
    {
        const auto size = win()->get_window_size<int>();
        if (w) *w = size.width;
        if (h) *h = size.height;
    }

    bool is_focused()
    {
        return cots::platform::has_flag(
            win()->get_screen_state(),
            cots::platform::screen_state::active);
    }

    void set_cursor_visible(const bool visible)
    {
        if (visible) win()->mouse.show_cursor();
        else         win()->mouse.hide_cursor();
    }

    void lock_cursor  () { win()->mouse.lock_to_window(); }
    void unlock_cursor() { win()->mouse.unlock(); }

    void request_quit()
    {
        if (const HWND hwnd = win()->get_window_handle())
        {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
    }

    void install(cots::module::services& s)
    {
        s.window.get_size           = &get_size;
        s.window.is_focused         = &is_focused;
        s.window.set_cursor_visible = &set_cursor_visible;
        s.window.lock_cursor        = &lock_cursor;
        s.window.unlock_cursor      = &unlock_cursor;
        s.window.request_quit       = &request_quit;
    }
}

COTS_INSTALL_SERVICES(install)