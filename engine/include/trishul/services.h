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
#ifndef CURSEOFTHESEA_SERVICES_H
#define CURSEOFTHESEA_SERVICES_H

#include "core/service_locator.h"

//~ services
#include "platform/platform_windows.h"

namespace trishul
{
    REGISTER_SERVICE(platform_window)
} // namespace trishul

#endif //CURSEOFTHESEA_SERVICES_H
