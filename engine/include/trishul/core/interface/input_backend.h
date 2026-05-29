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

namespace trishul::inputs
{
    struct input_initialize_info
    {
        HWND window_handle{ nullptr };
    };

    class __declspec(novtable) input_component: public interfaces::tickable
    {
    public:
        [[nodiscard]]
        virtual bool initialize  (const input_initialize_info& info) = 0;
        virtual void deinitialize() noexcept = 0;

        input_component           (const input_component&) = delete;
        input_component& operator=(const input_component&) = delete;

        input_component           (input_component&&) = delete;
        input_component& operator=(input_component&&) = delete;

        virtual bool poll_messages(
            UINT message,
            WPARAM w_param,
            LPARAM l_param
        ) noexcept = 0;

    protected:
        input_component() noexcept = default;
    };
} // namespace trishul::interface

#endif //CURSEOFTHESEA_INPUTS_H
