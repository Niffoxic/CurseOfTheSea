// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_PLATFORM_WINDOWS_H
#define CURSEOFTHESEA_PLATFORM_WINDOWS_H

#include <cstdint>
#include <string>
#include <type_traits>
#include <windows.h>

#include "components/keyboard_component.h"
#include "components/mouse_component.h"

#include "engine/core/framework/interface/subsystem.h"
#include "engine/core/engine_config.h"

namespace cots::platform
{
    enum class screen_state: std::uint8_t
    {
        none        = 0,
        windowed    = 1,
        fullscreen  = 1 << 1,
        minimized   = 1 << 2,
        inactive    = 1 << 3,
        active      = 1 << 4,
    };

    enum class status: std::uint8_t
    {
        Running = 0,
        Quit    = 1,
        Pause   = 2
    };

    //~ window configurations
    enum class window_style: std::uint8_t
    {
        normal      = 0, //~ Overlapped window
        borderless  = 1, // popup
    };

    constexpr screen_state operator|(
        screen_state lhs,
        screen_state rhs) noexcept
    {
        using T = std::underlying_type_t<screen_state>;
        return static_cast<screen_state>(static_cast<T>(lhs) | static_cast<T>(rhs));
    }

    constexpr screen_state operator&(
        screen_state lhs,
        screen_state rhs) noexcept
    {
        using T = std::underlying_type_t<screen_state>;
        return static_cast<screen_state>(static_cast<T>(lhs) & static_cast<T>(rhs));
    }

    constexpr screen_state operator~(screen_state rhs) noexcept
    {
        using T = std::underlying_type_t<screen_state>;
        return static_cast<screen_state>(~static_cast<T>(rhs));
    }

    constexpr screen_state operator^(
        screen_state left,
        screen_state right) noexcept
    {
        using T = std::underlying_type_t<screen_state>;
        return static_cast<screen_state>(static_cast<T>(left) ^ static_cast<T>(right));
    }

    constexpr screen_state& operator|=(screen_state& left, const screen_state right) noexcept
    {
        return left = left | right;
    }

    constexpr screen_state& operator&=(screen_state& left, const screen_state right) noexcept
    {
        return left = left & right;
    }

    constexpr screen_state& operator^=(screen_state& left, const screen_state right) noexcept
    {
        return left = left ^ right;
    }

    constexpr bool has_flag(
        const screen_state state,
        const screen_state check_with) noexcept
    {
        return (state & check_with) == check_with;
    }

    class windows final: public interfaces::subsystem ,public interfaces::tickable
    {
    public:
         windows() = default;
        ~windows() override;

        windows(const windows&) = delete;
        windows(windows&&)      = delete;

        windows& operator=(const windows&) = delete;
        windows& operator=(windows&&)      = delete;

        [[nodiscard]] bool initialize  () override;
                      void deinitialize() noexcept override;

        void begin_update(float delta_time) override;
        void end_update  () override;

        //~ components
        keyboard_component keyboard{};
        mouse_component    mouse   {};

        [[nodiscard]] HWND      get_window_handle() const noexcept;
        [[nodiscard]] HINSTANCE get_instance     () const noexcept;

        [[nodiscard]] screen_state get_screen_state() const noexcept;
        [[nodiscard]] status       get_status      () const noexcept;
        [[nodiscard]] bool         should_close    () const noexcept;

        template<typename T=int>
        requires std::is_integral_v<T>
        [[nodiscard]] config::size<T> get_window_size() const noexcept
        {
            return window_size_.as<T>();
        }

        //~ setters
        void set_style   (window_style style)                        const;
        void set_size    (std::uint32_t width, std::uint32_t height) const;
        void set_position(std::uint32_t x,     std::uint32_t y)      const;
        void set_title   (const std::wstring& title)                 const;
        void set_tile    (const std::string&  title)                 const;
        void set_debug   (const std::string&  message)               const;
        void set_debug   (const std::wstring& message)               const;

        //~ helpers
        [[nodiscard]] HICON load_icon(const std::wstring& path, int size) const noexcept;

    private:
        void create_window(const config::windows* info);

               LRESULT          handle_message   (HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);
        static LRESULT CALLBACK window_proc_setup(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);
        static LRESULT CALLBACK window_proc_thunk(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);

     private:
        static constexpr auto CLASS_NAME = L"COTS";

        mutable
        std::wstring        window_title_   { L"Cots" };
        config::size<int>   window_size_    {};
        HWND                window_handle_  { nullptr };
        HINSTANCE           window_instance_{ nullptr };
        screen_state        screen_state_   { screen_state::none };
        status              status_         { status::Running    };
    };
} // namespace cots

#endif //CURSEOFTHESEA_PLATFORM_WINDOWS_H
