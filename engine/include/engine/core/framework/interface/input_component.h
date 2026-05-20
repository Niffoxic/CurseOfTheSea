// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_INTERFACE_INPUT_COMPONENT_H
#define CURSEOFTHESEA_INTERFACE_INPUT_COMPONENT_H

#include <windows.h>
#include "tickable.h"

namespace cots::interfaces
{
    struct input_initialize_info
    {
        HWND window_handle{ nullptr };
    };

    class input_component: public tickable
    {
    public:
        [[nodiscard]] virtual bool initialize  (const input_initialize_info& info)  = 0;
                      virtual void deinitialize()                                   = 0;

        virtual bool poll_messages(
            UINT message,
            WPARAM w_param,
            LPARAM l_param) = 0;
    };
} // namespace cots::interface

#endif //CURSEOFTHESEA_INTERFACE_INPUT_COMPONENT_H
