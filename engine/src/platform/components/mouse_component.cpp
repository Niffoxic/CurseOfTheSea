// Created by Niffoxic (Harsh Dubey)
#include "engine/platform/components/mouse_component.h"
#include <windowsx.h>

namespace cots::platform
{
    namespace
    {
        constexpr float k_wheel_tick = static_cast<float>(WHEEL_DELTA); // 120

        mouse_button x_button_from(const WPARAM w_param) noexcept
        {
            const WORD which = GET_XBUTTON_WPARAM(w_param);
            return (which == XBUTTON1) ? mouse_button::x1 : mouse_button::x2;
        }
    } // namespace

    bool mouse_component::initialize(const interfaces::input_initialize_info& info)
    {
        hwnd_ = info.window_handle;
        clear();
        raw_buffer_.reserve(64);

        RAWINPUTDEVICE device{};
        device.usUsagePage = 0x01;
        device.usUsage     = 0x02;
        device.dwFlags     = 0;
        device.hwndTarget  = hwnd_;
        RegisterRawInputDevices(&device, 1, sizeof(device));

        return true;
    }

    void mouse_component::deinitialize()
    {
        unlock();
        if (!cursor_visible_) show_cursor();
        clear();
        hwnd_ = nullptr;
    }

    void mouse_component::begin_update(float dt)
    {
        pressed_        .reset();
        released_       .reset();
        double_clicked_ .reset();

        raw_delta_ = {};

        wheel_   = 0.0f;
        wheel_h_ = 0.0f;
    }

    void mouse_component::end_update()
    {
    }

    bool mouse_component::poll_messages(UINT message, WPARAM w_param, LPARAM l_param)
    {
        switch (message)
        {
        case WM_MOUSEMOVE:
            position_.x = GET_X_LPARAM(l_param);
            position_.y = GET_Y_LPARAM(l_param);
            return true;

        case WM_LBUTTONDOWN: on_button_down(mouse_button::left);   return true;
        case WM_LBUTTONUP:   on_button_up  (mouse_button::left);   return true;
        case WM_RBUTTONDOWN: on_button_down(mouse_button::right);  return true;
        case WM_RBUTTONUP:   on_button_up  (mouse_button::right);  return true;
        case WM_MBUTTONDOWN: on_button_down(mouse_button::middle); return true;
        case WM_MBUTTONUP:   on_button_up  (mouse_button::middle); return true;

        case WM_XBUTTONDOWN: on_button_down(x_button_from(w_param)); return true;
        case WM_XBUTTONUP:   on_button_up  (x_button_from(w_param)); return true;

        case WM_LBUTTONDBLCLK: on_double_click(mouse_button::left);          return true;
        case WM_RBUTTONDBLCLK: on_double_click(mouse_button::right);         return true;
        case WM_MBUTTONDBLCLK: on_double_click(mouse_button::middle);        return true;
        case WM_XBUTTONDBLCLK: on_double_click(x_button_from(w_param));      return true;

        case WM_MOUSEWHEEL:
            wheel_   += GET_WHEEL_DELTA_WPARAM(w_param) / k_wheel_tick;
            return true;
        case WM_MOUSEHWHEEL:
            wheel_h_ += GET_WHEEL_DELTA_WPARAM(w_param) / k_wheel_tick;
            return true;

        case WM_INPUT:
            handle_raw_input(l_param);
            return true;

        case WM_KILLFOCUS:
        case WM_SETFOCUS:
            clear();
            return false;

        default:
            return false;
        }
    }

    void mouse_component::on_button_down(mouse_button b) noexcept
    {
        if (!valid(b)) return;
        if (!down_.test(idx(b))) pressed_.set(idx(b));
        down_.set(idx(b));
    }

    void mouse_component::on_button_up(mouse_button b) noexcept
    {
        if (!valid(b)) return;
        if (down_.test(idx(b)))
        {
            released_   .set(idx(b));
            down_       .reset(idx(b));
        }
    }

    void mouse_component::on_double_click(mouse_button b) noexcept
    {
        if (!valid(b)) return;
        if (!down_.test(idx(b))) pressed_.set(idx(b));

        down_           .set(idx(b));
        double_clicked_ .set(idx(b));
    }

    void mouse_component::handle_raw_input(LPARAM l_param)
    {
        UINT size = 0;
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(l_param),
                            RID_INPUT, nullptr, &size,
                            sizeof(RAWINPUTHEADER)) != 0
                            || size == 0)
        {
            return;
        }

        if (raw_buffer_.size() < size) raw_buffer_.resize(size);

        const UINT got = GetRawInputData(reinterpret_cast<HRAWINPUT>(l_param),
                                         RID_INPUT, raw_buffer_.data(),
                                         &size, sizeof(RAWINPUTHEADER));
        if (got != size) return;

        const auto* raw = reinterpret_cast<const RAWINPUT*>(raw_buffer_.data());
        if (raw->header.dwType != RIM_TYPEMOUSE) return;

        if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
        {
            raw_delta_.x += raw->data.mouse.lLastX;
            raw_delta_.y += raw->data.mouse.lLastY;
        }
    }

    bool mouse_component::is_down(mouse_button b) const noexcept
    {
        return valid(b) && down_.test(idx(b));
    }
    bool mouse_component::is_up(mouse_button b) const noexcept
    {
        return valid(b) && !down_.test(idx(b));
    }
    bool mouse_component::pressed(mouse_button b) const noexcept
    {
        return valid(b) && pressed_.test(idx(b));
    }
    bool mouse_component::released(mouse_button b) const noexcept
    {
        return valid(b) && released_.test(idx(b));
    }
    bool mouse_component::double_clicked(mouse_button b) const noexcept
    {
        return valid(b) && double_clicked_.test(idx(b));
    }

    void mouse_component::show_cursor()
    {
        if (cursor_visible_) return;
        ::ShowCursor(TRUE);
        cursor_visible_ = true;
    }
    void mouse_component::hide_cursor()
    {
        if (!cursor_visible_) return;
        ::ShowCursor(FALSE);
        cursor_visible_ = false;
    }

    void mouse_component::lock_to_window() const
    {
        if (!hwnd_) return;

        RECT client{};
        if (!GetClientRect(hwnd_, &client)) return;

        POINT tl{ client.left,  client.top    };
        POINT br{ client.right, client.bottom };
        ClientToScreen(hwnd_, &tl);
        ClientToScreen(hwnd_, &br);

        const RECT screen_rect{ tl.x, tl.y, br.x, br.y };
        ClipCursor(&screen_rect);
    }

    void mouse_component::unlock() const
    {
        ClipCursor(nullptr);
    }

    void mouse_component::center_in_window() const
    {
        if (!hwnd_) return;

        RECT client{};
        if (!GetClientRect(hwnd_, &client)) return;

        POINT center{
            (client.right  - client.left) / 2,
            (client.bottom - client.top)  / 2
        };
        ClientToScreen(hwnd_, &center);
        SetCursorPos(center.x, center.y);
    }

    void mouse_component::clear() noexcept
    {
        down_           .reset();
        pressed_        .reset();
        released_       .reset();
        double_clicked_ .reset();

        raw_delta_ = {};

        wheel_     = 0.0f;
        wheel_h_   = 0.0f;
    }
} // namespace cots::platform
