// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_REGISTRY_H
#define CURSEOFTHESEA_REGISTRY_H

#include "feature_locator.h"
#include "engine/utils/timer.h"
#include "engine/platform/platform_windows.h"
#include "engine/utils/event_dispatcher.h"
#include "engine/audio/audio_system.h"

namespace cots::utils
{
    REGISTER_FEATURE(timer);
}

namespace cots::platform
{
    REGISTER_FEATURE(windows);
}

namespace cots::events
{
    REGISTER_FEATURE(dispatcher);
}

namespace cots::audio
{
    REGISTER_FEATURE(system);
}

#endif //CURSEOFTHESEA_REGISTRY_H
