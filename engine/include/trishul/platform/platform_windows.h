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
#ifndef CURSEOFTHESEA_PLATFORM_WINDOWS_H
#define CURSEOFTHESEA_PLATFORM_WINDOWS_H

#include <cstdint>
#include <type_traits>
#include <memory>

#include "trishul/core/interface/subsystems.h"
#include "trishul/core/interface/tickable.h"
#include "trishul/core/component_host.h"

#include "inputs/keyboard_component.h"
#include "inputs/mouse_component.h"

namespace trishul
{
    template<typename T>
    requires std::is_arithmetic_v<T>
    struct win_size
    {
        T width { T(1280) };
        T height{ T(720) };

        win_size() = default;
        win_size(T w, T h) : width(w), height(h)
        {}

        [[nodiscard]] T get_aspect_ratio() const noexcept
        {
            return width / height;
        }

        template<typename Type>
        requires std::is_arithmetic_v<Type>
        [[nodiscard]] auto as() const noexcept -> win_size<Type>
        {
            return win_size<Type>(Type(width), Type(height));
        }
    };

    enum class screen_state: std::uint8_t
    {
        none        = 0,
        windowed    = 1,
        fullscreen  = 1 << 1,
        minimized   = 1 << 2,
        inactive    = 1 << 3,
        active      = 1 << 4,
    };

    enum class platform_status: std::uint8_t
    {
        Running = 0,
        Quit    = 1,
        Pause   = 2
    };

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

    //~ component tags
    struct keyboard{};
    struct mouse   {};

    class platform_window;
} // namespace trishul

//~ scope each tag to its owning host
ENGINE_DECLARE_COMPONENT(trishul::keyboard, trishul::inputs::keyboard_component, trishul::platform_window);
ENGINE_DECLARE_COMPONENT(trishul::mouse,    trishul::inputs::mouse_component,    trishul::platform_window);

namespace trishul
{
    struct window_create_info
    {
        win_size<int> window_size{};
        std::wstring  window_title{L"Trishul Engine" };

        int          icon_resource_id{ 101 };
        std::wstring icon_path       {L"assets/icons/app.ico" };
    };

    class platform_window final
        : public interfaces::subsystems
        , public interfaces::tickable
    {
        ENGINE_GENERATE_COMPONENTS(platform_window, keyboard, mouse)

    public:
         platform_window();
        ~platform_window() noexcept override;

        platform_window(const platform_window&) = delete;
        platform_window(platform_window&&)      = delete;

        platform_window& operator=(const platform_window&) = delete;
        platform_window& operator=(platform_window&&)      = delete;

        void set_window_create_info(const window_create_info& info);

        [[nodiscard]] bool initialize  () override;
                      void deinitialize() noexcept override;

        void begin_update(float delta_time) override;
        void end_update  () override;

        //~ getters
        [[nodiscard]] HWND      get_window_handle() const noexcept;
        [[nodiscard]] HINSTANCE get_instance     () const noexcept;

        [[nodiscard]] screen_state    get_screen_state() const noexcept;
        [[nodiscard]] platform_status get_status      () const noexcept;
        [[nodiscard]] bool            should_close    () const noexcept;

        template<typename T=int>
        requires std::is_integral_v<T>
        [[nodiscard]] win_size<T> get_window_size() const noexcept
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
        [[nodiscard]] HICON load_icon_resource(int resource_id, int size) const noexcept;

    private:
        void create_window(const window_create_info* info);

               LRESULT          handle_message   (HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);
        static LRESULT CALLBACK window_proc_setup(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);
        static LRESULT CALLBACK window_proc_thunk(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);

     private:
        window_create_info create_info_{};

        static constexpr auto CLASS_NAME = L"COTS";

        mutable
        std::wstring    window_title_   { L"Cots" };
        win_size<int>   window_size_    {};
        HWND            window_handle_  { nullptr };
        HINSTANCE       window_instance_{ nullptr };
        screen_state    screen_state_   { screen_state::none };
        platform_status status_         { platform_status::Running };

    };
} // namespace trishul

#endif //CURSEOFTHESEA_PLATFORM_WINDOWS_H
