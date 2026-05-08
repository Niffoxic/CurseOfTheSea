// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_REGISTRY_H
#define CURSEOFTHESEA_REGISTRY_H

#include "feature_locator.h"
#include "../utils/timer.h"
#include "../platform/platform_windows.h"

namespace cots::utils
{
    REGISTER_FEATURE(timer);
}

namespace cots::platform
{
    REGISTER_FEATURE(windows);
}

#endif //CURSEOFTHESEA_REGISTRY_H
