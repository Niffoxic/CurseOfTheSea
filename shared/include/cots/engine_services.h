// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_ENGINE_SERVICES_H
#define CURSEOFTHESEA_ENGINE_SERVICES_H

namespace cots::module
{
    struct services
    {
        void (*log_info) (const char* msg);
        void (*log_warn) (const char* msg);
        void (*log_error)(const char* msg);

        //~ TODO: add service locator
    };
} // namespace cots::module


#endif //CURSEOFTHESEA_ENGINE_SERVICES_H
