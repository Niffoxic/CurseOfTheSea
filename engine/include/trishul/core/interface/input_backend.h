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
#ifndef CURSEOFTHESEA_INPUTS_H
#define CURSEOFTHESEA_INPUTS_H

#include <windows.h>
#include "tickable.h"

namespace trishul::interfaces
{
    struct input_initialize_info
    {
        HWND window_handle{ nullptr };
    };

    __interface window_input: tickable
    {
        [[nodiscard]]
        virtual bool initialize  (const input_initialize_info& info) noexcept;
        virtual void deinitialize() noexcept;

        virtual bool poll_messages(
            UINT message,
            WPARAM w_param,
            LPARAM l_param
        ) noexcept;
    };
} // namespace trishul::interface

#endif //CURSEOFTHESEA_INPUTS_H
