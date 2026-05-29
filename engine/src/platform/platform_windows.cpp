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
#include "trishul/platform/platform_windows.h"

#include "trishul/core/engine_assert.h"
#include "trishul/platform/inputs/keyboard_component.h"
#include "trishul/platform/inputs/mouse_component.h"

#include <spdlog/spdlog.h>

namespace trishul
{
    platform_window::platform_window()
        : interfaces::subsystems(std::string_view{ "platform_window" })
    {}

    platform_window::~platform_window() noexcept
    {
        deinitialize();
    }

    void platform_window::set_window_create_info(const window_create_info& info)
    {
        create_info_ = info;
    }

    bool platform_window::initialize()
    {
        construct_component<keyboard>();
        construct_component<mouse>();

        create_window(&create_info_);
        ENGINE_VERIFY_MSG(window_handle_ != nullptr, "failed to create window");

        inputs::input_initialize_info input_info{};
        input_info.window_handle = window_handle_;

        if (!get_component<keyboard>().initialize(input_info))
        {
            spdlog::error("failed to initialize keyboard");
            return false;
        }

        if (!get_component<mouse>().initialize(input_info))
        {
            spdlog::error("failed to initialize mouse");
            return false;
        }

        return true;
    }

    void platform_window::deinitialize() noexcept
    {
        //~ tear the window down first so wm_destroy still sees live components
        if (window_handle_)
        {
            DestroyWindow(window_handle_);
            window_handle_ = nullptr;
        }

        if (has_component<mouse>())    get_component<mouse>()   .deinitialize();
        if (has_component<keyboard>()) get_component<keyboard>().deinitialize();

        if (window_instance_)
        {
            UnregisterClassW(CLASS_NAME, window_instance_);
            window_instance_ = nullptr;
        }
    }

    void platform_window::begin_update(const float delta_time)
    {
        get_component<keyboard>().begin_update(delta_time);
        get_component<mouse>()   .begin_update(delta_time);

        //~ drain the queue
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void platform_window::end_update()
    {
        get_component<keyboard>().end_update();
        get_component<mouse>()   .end_update();
    }

    HWND platform_window::get_window_handle() const noexcept
    {
        return window_handle_;
    }

    HINSTANCE platform_window::get_instance() const noexcept
    {
        return window_instance_;
    }

    screen_state platform_window::get_screen_state() const noexcept
    {
        return screen_state_;
    }

    platform_status platform_window::get_status() const noexcept
    {
        return status_;
    }

    bool platform_window::should_close() const noexcept
    {
        return status_ == platform_status::Quit;
    }

    void platform_window::set_style(const window_style style) const
    {
        if (!window_handle_) return;

        const DWORD dw_style = (style == window_style::borderless)
            ? (WS_POPUP | WS_VISIBLE)
            : WS_OVERLAPPEDWINDOW;

        SetWindowLongPtrW(window_handle_, GWL_STYLE, static_cast<LONG_PTR>(dw_style));
        SetWindowPos(window_handle_, HWND_TOP, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
    }

    void platform_window::set_size(const std::uint32_t width, const std::uint32_t height) const
    {
        if (!window_handle_) return;
        SetWindowPos(window_handle_, nullptr, 0, 0,
                     static_cast<int>(width), static_cast<int>(height),
                     SWP_NOMOVE | SWP_NOZORDER);
    }

    void platform_window::set_position(const std::uint32_t x, const std::uint32_t y) const
    {
        if (!window_handle_) return;
        SetWindowPos(window_handle_, nullptr,
                     static_cast<int>(x), static_cast<int>(y), 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER);
    }

    void platform_window::set_title(const std::wstring& title) const
    {
        if (!window_handle_) return;
        window_title_ = title;
        SetWindowTextW(window_handle_, title.c_str());
    }

    void platform_window::set_tile(const std::string& title) const
    {
        if (!window_handle_) return;
        window_title_ = std::wstring(title.begin(), title.end());
        SetWindowTextA(window_handle_, title.c_str());
    }

    void platform_window::set_debug(const std::string& message) const
    {
        if (!window_handle_) return;
        const auto bar = std::string(window_title_.begin(), window_title_.end()) + " - " + message;
        SetWindowTextA(window_handle_, bar.c_str());
    }

    void platform_window::set_debug(const std::wstring& message) const
    {
        if (!window_handle_) return;
        const auto bar = window_title_ + L" - " + message;
        SetWindowTextW(window_handle_, bar.c_str());
    }

    HICON platform_window::load_icon(const std::wstring& path, const int size) const noexcept
    {
        return static_cast<HICON>(LoadImageW(
            nullptr,
            path.c_str(),
            IMAGE_ICON,
            size, size,
            LR_LOADFROMFILE | LR_DEFAULTCOLOR));
    }

    HICON platform_window::load_icon_resource(const int resource_id, const int size) const noexcept
    {
        if (resource_id <= 0) return nullptr;

        const HMODULE exe = GetModuleHandleW(nullptr);
        return static_cast<HICON>(LoadImageW(
            exe,
            MAKEINTRESOURCEW(resource_id),
            IMAGE_ICON,
            size, size,
            LR_DEFAULTCOLOR));
    }

    void platform_window::create_window(const window_create_info* info)
    {
        window_instance_ = GetModuleHandleW(nullptr);
        window_title_    = info->window_title;
        window_size_     = info->window_size;

        WNDCLASSEXW window_class{};
        window_class.cbSize        = sizeof(WNDCLASSEXW);
        window_class.style         = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc   = window_proc_setup;
        window_class.hInstance     = window_instance_;
        window_class.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        window_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        window_class.lpszClassName = CLASS_NAME;

        const int small_size = GetSystemMetrics(SM_CXSMICON);
        window_class.hIcon    = load_icon_resource(info->icon_resource_id, 32);
        window_class.hIconSm  = load_icon_resource(info->icon_resource_id, small_size);

        if (!window_class.hIcon)   window_class.hIcon   = load_icon(info->icon_path, 32);
        if (!window_class.hIconSm) window_class.hIconSm = load_icon(info->icon_path, small_size);

        if (!window_class.hIcon)   window_class.hIcon   = LoadIconW(nullptr, IDI_APPLICATION);
        if (!window_class.hIconSm) window_class.hIconSm = window_class.hIcon;

        window_class.cbWndExtra   = 0;
        window_class.cbClsExtra   = 0;
        window_class.lpszMenuName = nullptr;

        ENGINE_VERIFY_MSG(RegisterClassExW(&window_class) != 0,
            "failed to register window class");

        constexpr DWORD style    = WS_OVERLAPPEDWINDOW;
        constexpr DWORD ex_style = WS_EX_APPWINDOW;

        RECT rt{};
        rt.left   = 0;
        rt.top    = 0;
        rt.right  = info->window_size.as<LONG>().width;
        rt.bottom = info->window_size.as<LONG>().height;

        if (!AdjustWindowRectEx(&rt, style, FALSE, ex_style))
        {
            spdlog::warn("failed to adjust window rect window might size oddly");
        }

        const win_size<int> full_size{ rt.right - rt.left, rt.bottom - rt.top };

        window_handle_ = CreateWindowExW(
            ex_style, CLASS_NAME,
            info->window_title.c_str(),
            style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            full_size.as<LONG>().width,
            full_size.as<LONG>().height,
            nullptr, nullptr,
            window_instance_, this);

        ENGINE_VERIFY_MSG(window_handle_ != nullptr, "failed to create window");

        ShowWindow(window_handle_, SW_SHOW);
        UpdateWindow(window_handle_);

        screen_state_ = screen_state::windowed | screen_state::active;
        status_       = platform_status::Running;
    }

    LRESULT platform_window::handle_message(
        HWND hwnd, const UINT message,
        const WPARAM w_param, const LPARAM l_param)
    {
        get_component<mouse>()   .poll_messages(message, w_param, l_param);
        get_component<keyboard>().poll_messages(message, w_param, l_param);

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
                window_size_.width  = static_cast<int>(width);
                window_size_.height = static_cast<int>(height);

                //~ TODO: publish resize once the dispatcher exists
            }
            return 0;
        }
        case WM_ACTIVATE:
        {
            if (LOWORD(w_param) == WA_INACTIVE)
            {
                screen_state_ &= ~screen_state::active;
                screen_state_ |=  screen_state::inactive;
            }
            else
            {
                screen_state_ |=  screen_state::active;
                screen_state_ &= ~screen_state::inactive;
            }
            return 0;
        }
        case WM_DESTROY:
            status_ = platform_status::Quit;
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, message, w_param, l_param);
        }
    }

    LRESULT CALLBACK platform_window::window_proc_setup(
        HWND hwnd, const UINT message,
        const WPARAM w_param, const LPARAM l_param)
    {
        if (message == WM_NCCREATE)
        {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
            auto* that = static_cast<platform_window*>(create->lpCreateParams);

            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC,  reinterpret_cast<LONG_PTR>(&window_proc_thunk));

            return that->handle_message(hwnd, message, w_param, l_param);
        }
        return DefWindowProcW(hwnd, message, w_param, l_param);
    }

    LRESULT CALLBACK platform_window::window_proc_thunk(
        HWND hwnd, const UINT message,
        const WPARAM w_param, const LPARAM l_param)
    {
        if (auto* that = reinterpret_cast<platform_window*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA)))
        {
            return that->handle_message(hwnd, message, w_param, l_param);
        }
        return DefWindowProcW(hwnd, message, w_param, l_param);
    }
} // namespace trishul
