// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_KEYBOARD_COMPONENT_H
#define CURSEOFTHESEA_KEYBOARD_COMPONENT_H

#include <bitset>
#include <initializer_list>
#include <string>
#include <type_traits>
#include "engine/core/framework/interface/input_component.h"

namespace cots::platform
{
    enum class key_mode
    {
        none    = 0,
        ctrl    = 1,
        shift   = 1 << 1,
        alt     = 1 << 2,
        super   = 1 << 3
    };

    constexpr key_mode operator|(const key_mode left, const key_mode right) noexcept
    {
        using T = std::underlying_type_t<key_mode>;
        return static_cast<key_mode>(static_cast<T>(left) | static_cast<T>(right));
    }

    constexpr key_mode operator&(const key_mode left, const key_mode right) noexcept
    {
        using T = std::underlying_type_t<key_mode>;
        return static_cast<key_mode>(static_cast<T>(left) & static_cast<T>(right));
    }

    constexpr key_mode operator~(const key_mode rhs) noexcept
    {
        using T = std::underlying_type_t<key_mode>;
        return static_cast<key_mode>(~static_cast<T>(rhs));
    }

    constexpr key_mode operator^(const key_mode left, const key_mode right) noexcept
    {
        using T = std::underlying_type_t<key_mode>;
        return static_cast<key_mode>(static_cast<T>(left) ^ static_cast<T>(right));
    }

    constexpr key_mode& operator|=(key_mode &left, const key_mode right) noexcept
    {
        return left = left | right;
    }

    constexpr key_mode& operator&=(key_mode &left, const key_mode right) noexcept
    {
        return left = left & right;
    }

    constexpr key_mode& operator^=(key_mode &left, const key_mode right) noexcept
    {
        return left = left ^ right;
    }

    constexpr bool has_flag(const key_mode state,
                            const key_mode check_with) noexcept
    {
        return (state & check_with) == check_with;
    }

    class keyboard_component final: public interfaces::input_component
    {
    public:
        static constexpr std::size_t key_count = 256;

         keyboard_component()          = default;
        ~keyboard_component() override = default;

        keyboard_component(const keyboard_component&) = delete;
        keyboard_component(keyboard_component&&)      = default;

        keyboard_component& operator=(const keyboard_component&) = delete;
        keyboard_component& operator=(keyboard_component&&)      = default;

        [[nodiscard]] bool initialize(const interfaces::input_initialize_info &info) override;

        void begin_update(float delta_time) override;

        void deinitialize() override;
        void end_update  () override;


        bool poll_messages(
            UINT message, WPARAM w_param,
            LPARAM l_param) override;

        //~ continuous state queries
        [[nodiscard]] bool is_down(int vk) const noexcept;
        [[nodiscard]] bool is_up  (int vk) const noexcept;

        //~ one frame edge queries
        [[nodiscard]] bool pressed (int vk) const noexcept;
        [[nodiscard]] bool released(int vk) const noexcept;

        //~ combos

        // chord: virtual key pressed with `key mode` held
        // strict(true): means crtl+s is requested then ctrl+s+shift will not fire
        [[nodiscard]] bool chord(
            int vk,
            key_mode mode = key_mode::none,
            bool strict=true) const noexcept;

        [[nodiscard]] bool all_down(std::initializer_list<int> keys) const noexcept;
        [[nodiscard]] bool any_down(std::initializer_list<int> keys) const noexcept;

        //~ debug or test press
        [[nodiscard]] bool any_pressed() const noexcept;

        //~ modifiers
        [[nodiscard]] bool ctrl_down () const noexcept;
        [[nodiscard]] bool shift_down() const noexcept;
        [[nodiscard]] bool alt_down  () const noexcept;
        [[nodiscard]] bool super_down() const noexcept;

        [[nodiscard]] key_mode active_mods() const noexcept;

        //~ utility
        void clear() noexcept;

        //~ helpers
        [[nodiscard]] static constexpr bool valid(const int vk) noexcept
        {
            return vk >= 0 && vk < key_count;
        }

        [[nodiscard]] static constexpr bool repeat(const LPARAM l_param) noexcept
        {
            return (l_param & (1u << 30)) != 0;
        }

    private:
        std::bitset<key_count> down_    {};
        std::bitset<key_count> pressed_ {};
        std::bitset<key_count> released_{};
    };
} // namespace cots::platform

#endif //CURSEOFTHESEA_KEYBOARD_COMPONENT_H
