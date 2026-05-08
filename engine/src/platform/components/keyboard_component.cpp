// Created by Niffoxic (Harsh Dubey)
#include "engine/platform/components/keyboard_component.h"

#include <algorithm>

void cots::platform::keyboard_component::begin_update(float delta_time)
{
    //~ not needed
}

void cots::platform::keyboard_component::end_update()
{
    pressed_ .reset();
    released_.reset();
}

bool cots::platform::keyboard_component::initialize(const interface::input_initialize_info &info)
{
    clear();
    return true;
}

void cots::platform::keyboard_component::deinitialize()
{
    clear();
}

bool cots::platform::keyboard_component::poll_messages(
    const UINT message,
    const WPARAM w_param,
    const LPARAM l_param)
{
    switch (message)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const int vk = static_cast<int>(w_param);
        if (!valid(vk)) return false;

        // only fire press edge on real transitions not auto repeat
        if (!down_.test(vk) && !repeat(l_param))
            pressed_.set(vk);
        down_.set(vk);
        return true;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        const int vk = static_cast<int>(w_param);
        if (!valid(vk)) return false;

        if (down_.test(vk))
        {
            released_.set(vk);
            down_.reset(vk);
        }
        return true;
    }
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
        // prevent stuck keys when the window loses gains focus
        clear();
        return false;
    default:
        return false;
    }
}

bool cots::platform::keyboard_component::is_down(int vk) const noexcept
{
    return valid(vk) && down_.test(vk);
}

bool cots::platform::keyboard_component::is_up(int vk) const noexcept
{
    return valid(vk) && !down_.test(vk);
}

bool cots::platform::keyboard_component::pressed(int vk) const noexcept
{
    return valid(vk) && pressed_.test(vk);
}

bool cots::platform::keyboard_component::released(int vk) const noexcept
{
    return valid(vk) && released_.test(vk);
}

bool cots::platform::keyboard_component::chord(
    const int vk, const key_mode mode,
    const bool strict) const noexcept
{
    if (!pressed(vk)) return false;

    const auto active = active_mods();

    if (!has_flag(active, mode))        return false;
    if (strict && (active & ~mode) != key_mode::none) return false;

    return true;
}

bool cots::platform::keyboard_component::all_down(std::initializer_list<int> keys) const noexcept
{
    if (keys.size() == 0) return false;

    return std::ranges::all_of(keys,
        [this](const int vk)
        {
            return is_down(vk);
        });
}

bool cots::platform::keyboard_component::any_down(std::initializer_list<int> keys) const noexcept
{
    if (keys.size() == 0) return false;

    return std::ranges::any_of(keys,
        [this](const int vk)
        {
            return is_down(vk);
        });
}

bool cots::platform::keyboard_component::any_pressed() const noexcept
{
    return pressed_.any();
}

bool cots::platform::keyboard_component::ctrl_down() const noexcept
{
    return down_.test(VK_CONTROL) || down_.test(VK_LCONTROL) || down_.test(VK_RCONTROL);
}

bool cots::platform::keyboard_component::shift_down() const noexcept
{
    return down_.test(VK_SHIFT) || down_.test(VK_LSHIFT) || down_.test(VK_RSHIFT);
}

bool cots::platform::keyboard_component::alt_down() const noexcept
{
    return down_.test(VK_MENU) || down_.test(VK_LMENU) || down_.test(VK_RMENU);
}

bool cots::platform::keyboard_component::super_down() const noexcept
{
    return down_.test(VK_LWIN) || down_.test(VK_RWIN);
}

cots::platform::key_mode cots::platform::keyboard_component::active_mods() const noexcept
{
    auto m = key_mode::none;
    if (ctrl_down ()) m |= key_mode::ctrl;
    if (shift_down()) m |= key_mode::shift;
    if (alt_down  ()) m |= key_mode::alt;
    if (super_down()) m |= key_mode::super;
    return m;
}

void cots::platform::keyboard_component::clear() noexcept
{
    down_    .reset();
    pressed_ .reset();
    released_.reset();
}
