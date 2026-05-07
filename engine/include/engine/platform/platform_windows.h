// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_PLATFORM_WINDOWS_H
#define CURSEOFTHESEA_PLATFORM_WINDOWS_H

#include <cstdint>
#include <string>
#include <type_traits>
#include <windows.h>

// TODO: Add Subsystem Interface and Subsystem manager
// TODO: Add Tickable Interface
//

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

    template<typename T>
    requires std::is_arithmetic_v<T>
    struct size
    {
        T width { T(1280) };
        T height{ T(720) };

        size() = default;
        size(T w, T h) : width(w), height(h)
        {}

        [[nodiscard]] T get_aspect_ratio() const noexcept
        {
            return width / height;
        }

        template<typename Type>
        requires std::is_arithmetic_v<Type>
        [[nodiscard]] auto as() const noexcept -> size<Type>
        {
            return size<Type>(Type(width), Type(height));
        }
    };

    struct initialize_info
    {
        size<int>    window_size{};
        std::wstring window_title{L"COTS Engine" };
        std::wstring icon_path   {L"assets/icons/app.ico" };
    };

    class windows
    {
    public:
         windows() = default;
        ~windows();

        windows(const windows&) = delete;
        windows(windows&&)      = delete;

        windows& operator=(const windows&) = delete;
        windows& operator=(windows&&)      = delete;

        [[nodiscard]] bool initialize(const initialize_info& info);
                      void deinitialize() noexcept;

        void begin_frame(float delta_time);
        void end_frame  ();

        [[nodiscard]] HWND      get_window_handle() const noexcept;
        [[nodiscard]] HINSTANCE get_instance     () const noexcept;

        [[nodiscard]] screen_state get_screen_state() const noexcept;
        [[nodiscard]] status       get_status      () const noexcept;
        [[nodiscard]] bool         should_close    () const noexcept;

        template<typename T=int>
        requires std::is_integral_v<T>
        [[nodiscard]] size<T> get_window_size() const noexcept
        {
            return window_size_.as<T>();
        }

        //~ helpers
        [[nodiscard]] HICON load_icon(const std::wstring& path, int size) const noexcept;

    private:
        void create_window(const initialize_info& info);

               LRESULT          handle_message   (HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);
        static LRESULT CALLBACK window_proc_setup(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);
        static LRESULT CALLBACK window_proc_thunk(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);

    private:
        static constexpr auto CLASS_NAME = L"COTS";

        size<int>    window_size_    {};
        HWND         window_handle_  { nullptr };
        HINSTANCE    window_instance_{ nullptr };
        screen_state screen_state_   { screen_state::none };
        status       status_         { status::Running };
    };
} // namespace cots

#endif //CURSEOFTHESEA_PLATFORM_WINDOWS_H
