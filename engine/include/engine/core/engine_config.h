// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_ENGINE_CONFIG_H
#define CURSEOFTHESEA_ENGINE_CONFIG_H

#include <string>

namespace cots::config
{
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

    enum class window_style: std::uint8_t
    {
        normal      = 0, //~ Overlapped window
        borderless  = 1, // popup
    };

    struct windows
    {
        size<int>    window_size{};
        std::wstring window_title{L"COTS Engine" };
        std::wstring icon_path   {L"assets/icons/app.ico" };
    };

    class manager
    {
    public:
         manager() = default;
        ~manager() = default;

                            windows& windows_config()       { return windows_; }
        [[nodiscard]] const windows& windows_config() const { return windows_; }

    private:
        windows windows_{};
    };
} // namespace

#endif //CURSEOFTHESEA_ENGINE_CONFIG_H
