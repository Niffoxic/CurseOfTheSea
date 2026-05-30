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
#ifndef CURSEOFTHESEA_DEVICE_EVENT_H
#define CURSEOFTHESEA_DEVICE_EVENT_H

namespace trishul::events
{
    struct device_recreating{}; //~ called before device is being recreated

    struct device_recreated //~ called just after device is recreated
    {
        std::uint32_t adapter_index{};
    };

    struct device_lost //~ GG you crashed it AGAIN!!
    {
        HRESULT removal_reason;
    };
} // namespace trishul::events

#endif //CURSEOFTHESEA_DEVICE_EVENT_H
