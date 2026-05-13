// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SWAPCHAIN_H
#define CURSEOFTHESEA_SWAPCHAIN_H

#include <cstdint>

namespace cots::graphics::hardware
{
    class device;
    struct swapchain_create_info
    {
        device* device_{ nullptr };
        uint32_t width { 0 };
        uint32_t height{ 0 };
    }; // swapchain create info

    class swapchain final
    {
    public:
         swapchain() = default;
        ~swapchain();

        [[nodiscard]]
        bool initialize(const swapchain_create_info& info);
        void deinitialize() noexcept;


    };
} // namespace cots::graphics::hardware

#endif //CURSEOFTHESEA_SWAPCHAIN_H
