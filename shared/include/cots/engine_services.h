// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_ENGINE_SERVICES_H
#define CURSEOFTHESEA_ENGINE_SERVICES_H

namespace cots::module
{
    struct log_services
    {
        void (*info) (const char* msg);
        void (*warn) (const char* msg);
        void (*error)(const char* msg);
    };

    struct window_services
    {
        void (*get_size)(int* width, int* height);
    };

    struct input_services
    {

    };

    struct services
    {
        log_services    log;
        window_services window;
        input_services  input;
    };
} // namespace cots::module


#endif //CURSEOFTHESEA_ENGINE_SERVICES_H
