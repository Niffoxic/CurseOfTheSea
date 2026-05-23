#include "engine/platform/platform_windows.h"

#include <filesystem>

#include "engine/events/event_dispatcher.h"
#include "engine/events/windows_event.h"
#include "engine/system/feature_locator.h"
#include "spdlog/spdlog.h"

#if COTS_EDITOR_ENABLED
#include "editor/editor.h"
#endif

namespace
{
    //~ message is a mouse event
    constexpr bool is_mouse_message(const UINT m) noexcept
    {
        return (m >= WM_MOUSEFIRST && m <= WM_MOUSELAST) || m == WM_INPUT;
    }

    //~ message is a keyboard event
    constexpr bool is_keyboard_message(const UINT m) noexcept
    {
        return (m >= WM_KEYFIRST && m <= WM_KEYLAST);
    }
} // namespace

namespace cots::platform
{
    windows::~windows()
    {
        deinitialize();
    }

    bool windows::initialize()
    {
        const auto info = reinterpret_cast<const config::windows*>(config_);
        create_window(info);
        COTS_ASSERT_MSG(window_handle_ != nullptr, "Failed to create window");

        interfaces::input_initialize_info input_info{};
        input_info.window_handle = window_handle_;

        if (not keyboard.initialize(input_info))
        {
            spdlog::error("Failed to initialize keyboard");
            return false;
        }

        if (not mouse.initialize(input_info))
        {
            spdlog::error("Failed to initialize mouse");
            return false;
        }

        return true;
    }

    void windows::deinitialize() noexcept
    {
        if (window_instance_)
        {
            UnregisterClassW(CLASS_NAME, window_instance_);
        }
    }

    void windows::begin_update(const float delta_time)
    {
        keyboard.begin_update(delta_time);
        mouse   .begin_update(delta_time);

        //~ poll messages
        MSG msg{};

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void windows::end_update()
    {
        keyboard.end_update();
        mouse   .end_update();
    }

    HWND windows::get_window_handle() const noexcept
    {
        return window_handle_;
    }

    HINSTANCE windows::get_instance() const noexcept
    {
        return window_instance_;
    }

    screen_state windows::get_screen_state() const noexcept
    {
        return screen_state_;
    }

    status windows::get_status() const noexcept
    {
        return status_;
    }

    bool windows::should_close() const noexcept
    {
        return status_ == status::Quit;
    }

    void windows::set_style(const window_style style) const
    {
        if (!window_handle_) return;

        const DWORD dw_style = (style == window_style::borderless)
            ? (WS_POPUP | WS_VISIBLE)
            : WS_OVERLAPPEDWINDOW;

        SetWindowLongPtrW(window_handle_, GWL_STYLE, dw_style);
        SetWindowPos(window_handle_, HWND_TOP, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
    }

    void windows::set_size(const std::uint32_t width, const std::uint32_t height) const
    {
        if (!window_handle_) return;
        SetWindowPos(window_handle_, nullptr, 0, 0,
                     static_cast<int>(width), static_cast<int>(height),
                     SWP_NOMOVE | SWP_NOZORDER);
    }

    void windows::set_position(const std::uint32_t x, const std::uint32_t y) const
    {
        if (!window_handle_) return;
        SetWindowPos(window_handle_, nullptr, x, y, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER);
    }

    void windows::set_title(const std::wstring& title) const
    {
        if (!window_handle_) return;
        window_title_ = title;
        SetWindowTextW(window_handle_, title.c_str());
    }

    void windows::set_tile(const std::string &title) const
    {
        if (!window_handle_) return;
        window_title_ = std::wstring(title.begin(), title.end());
        SetWindowTextA(window_handle_, title.c_str());
    }

    void windows::set_debug(const std::string &message) const
    {
        if (!window_handle_) return;
        const auto bar = std::string(window_title_.begin(), window_title_.end()) + " - " + message;
        SetWindowTextA(window_handle_, bar.c_str());
    }

    void windows::set_debug(const std::wstring &message) const
    {
        if (!window_handle_) return;
        const auto bar = window_title_ + L" - " + message;
        SetWindowTextW(window_handle_, bar.c_str());
    }

    HICON windows::load_icon(const std::wstring &path, const int size) const noexcept
    {
        return static_cast<HICON>(LoadImageW(
              nullptr,
              path.c_str(),
              IMAGE_ICON,
              size, size,
              LR_LOADFROMFILE | LR_DEFAULTCOLOR
          ));
    }

    void windows::create_window(const config::windows* info)
    {
        WNDCLASSEXW window_class{};
        window_class.cbSize        = sizeof(WNDCLASSEX);
        window_class.style         = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc   = window_proc_setup;
        window_class.hInstance     = window_instance_;
        window_class.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        window_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        window_class.lpszClassName = CLASS_NAME;

        window_class.hIcon   = load_icon(info->icon_path, 32);
        window_class.hIconSm = load_icon(info->icon_path, GetSystemMetrics(SM_CXSMICON));

        if (!window_class.hIcon)
        {
            window_class.hIcon   = LoadIconW(nullptr, IDI_APPLICATION);
            window_class.hIconSm = window_class.hIcon;
        }

        window_class.cbWndExtra    = 0;
        window_class.cbClsExtra    = 0;
        window_class.lpszMenuName  = nullptr;

        if (not RegisterClassExW(&window_class))
        {
            spdlog::error("Failed to register window class");
            throw std::runtime_error("Failed to register window class");
        }

        constexpr DWORD style    = WS_OVERLAPPEDWINDOW;
        constexpr DWORD ex_style = WS_EX_APPWINDOW;

        RECT rt{};
        rt.left   = 0;
        rt.top    = 0;
        rt.right  = info->window_size.as<LONG>().width;
        rt.bottom = info->window_size.as<LONG>().height;

        if (not AdjustWindowRectEx(&rt, style, FALSE, ex_style))
        {
            spdlog::warn("Failed to adjust window rect window might work weird");
        }
        //~ updated size
        const config::size<int> window_size{
            rt.right - rt.left,
            rt.bottom - rt.top
        };

        //~ create window
        window_handle_ = CreateWindowExW(
            ex_style, CLASS_NAME,
            info->window_title.c_str(),
            style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            window_size.as<LONG>().width,
            window_size.as<LONG>().height,
            nullptr, nullptr,
            window_instance_, this
        );
        COTS_ASSERT_MSG(window_handle_ != nullptr, "Failed to create window");
        ShowWindow(window_handle_, SW_SHOW);
        UpdateWindow(window_handle_);

        screen_state_ = screen_state::windowed | screen_state::active;
        status_       = status::Running;
    }

    LRESULT windows::handle_message(
        HWND hwnd, const UINT message,
        const WPARAM w_param, const LPARAM l_param)
    {
#if COTS_EDITOR_ENABLED
        //~ hand the message to the editor queue
        editor::queue_win32_message(hwnd, message, w_param, l_param);

        const bool capture_mouse = editor::want_capture_mouse();
        const bool capture_kbd   = editor::want_capture_keyboard();

        if (!(capture_mouse && is_mouse_message(message)))
        {
            mouse.poll_messages(message, w_param, l_param);
        }
        if (!(capture_kbd && is_keyboard_message(message)))
        {
            keyboard.poll_messages(message, w_param, l_param);
        }
#else
        keyboard.poll_messages(message, w_param, l_param);
        mouse   .poll_messages(message, w_param, l_param);
#endif

        switch (message)
        {
        case WM_SIZE:
        {
            const auto width  = LOWORD(l_param);
            const auto height = HIWORD(l_param);

            if (w_param == SIZE_MINIMIZED)
            {
                screen_state_ |= screen_state::minimized;
            }
            else
            {
                screen_state_ &= ~screen_state::minimized;
                window_size_.width  = width;
                window_size_.height = height;

                //~ publish event for renderer
                if (const auto d = feature::locator::resolve<events::dispatcher>())
                {
                    d->enqueue<events::window_resized>(
                        static_cast<std::uint32_t>(width),
                        static_cast<std::uint32_t>(height));
                }
            }
            return 0;
        }
        case WM_ACTIVATE:
        {
            if (LOWORD(w_param) == WA_INACTIVE)
            {
                screen_state_ &= ~screen_state::active;
                screen_state_ |= screen_state::inactive;
            }
            else
            {
                screen_state_ |= screen_state::active;
                screen_state_ &= ~screen_state::inactive;
            }
            return 0;
        }
        case WM_DESTROY:
            status_ = status::Quit;
            spdlog::info("should be quitting");
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, message, w_param, l_param);
        }
    }

    LRESULT windows::window_proc_setup(
    HWND hwnd, const UINT message,
        const WPARAM w_param, const LPARAM l_param)
    {
        if (message == WM_NCCREATE)
        {
            const auto* create = reinterpret_cast<CREATESTRUCT*>(l_param);
            auto* that = static_cast<windows*>(create->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&window_proc_thunk));
            return that->handle_message(hwnd, message, w_param, l_param);
        }
        return DefWindowProcW(hwnd, message, w_param, l_param);
    }

    LRESULT windows::window_proc_thunk(
        HWND hwnd, const UINT message,
        const WPARAM w_param, const LPARAM l_param)
    {
        if (auto* that = reinterpret_cast<windows*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
        {
            return that->handle_message(hwnd, message, w_param, l_param);
        }

        return DefWindowProc(hwnd, message, w_param, l_param);
    }
} // namespace cots::platform
