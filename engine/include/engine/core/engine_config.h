// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_ENGINE_CONFIG_H
#define CURSEOFTHESEA_ENGINE_CONFIG_H

#include <string>

namespace cots::config
{
    inline static constexpr std::uint32_t SWAPCHAIN_BUFFER_COUNT = 2u;
    inline static constexpr std::uint32_t DEFAULT_ENGINE_FPS     = 360;

    //~ general
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

    struct windows
    {
        size<int>    window_size{};
        std::wstring window_title{ L"COTS Engine" };
        std::wstring icon_path   { L"assets/icons/app.ico" };
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
} // namespace cots::config

#endif //CURSEOFTHESEA_ENGINE_CONFIG_H
