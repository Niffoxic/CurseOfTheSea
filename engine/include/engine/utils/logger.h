// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_LOGGER_H
#define CURSEOFTHESEA_LOGGER_H

#include "engine/core/framework/interface/singleton.h"

namespace cots::utils
{
    class logger final: public interfaces::singleton<logger>
    {
        COTS_SINGLETON(logger)
         logger() = default;
        ~logger() = default;
    public:
    };
} // namespace cots::utils

#endif //CURSEOFTHESEA_LOGGER_H
