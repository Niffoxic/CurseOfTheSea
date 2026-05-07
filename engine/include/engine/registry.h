// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_REGISTRY_H
#define CURSEOFTHESEA_REGISTRY_H

#include "service_locator.h"
#include "utils/timer.h"
#include "platform/platform_windows.h"

namespace cots::utils
{
    REGISTER_SERVICE(timer);
}

namespace cots::platform
{
    REGISTER_SERVICE(windows);
}

#endif //CURSEOFTHESEA_REGISTRY_H
