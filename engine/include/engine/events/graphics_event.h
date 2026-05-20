// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GRAPHICS_EVENT_H
#define CURSEOFTHESEA_GRAPHICS_EVENT_H

#include <wrl/client.h>
#include <cstdint>

namespace cots::events::graphics
{
    struct device_creation_attempted{};

    struct device_validated
    {
        std::uint32_t adapter_index;
    };

    struct device_lost
    {
        HRESULT removal_reason;
    };
} // namespace cots::events::graphics

#endif //CURSEOFTHESEA_GRAPHICS_EVENT_H
