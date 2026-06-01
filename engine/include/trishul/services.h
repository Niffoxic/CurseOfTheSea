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
#include "utils/timer.h"
#include "event/dispatcher.h"
#include "renderer/render.h"
#include "trishul/renderer/mesh/mesh_registry.h"

namespace trishul
{
    REGISTER_SERVICE(platform_window)
    REGISTER_SERVICE(timer_manager)

    namespace events
    {
        REGISTER_SERVICE(dispatcher)
    } // namespace events

    namespace render
    {
        REGISTER_SERVICE(graphics)

        namespace mesh
        {
            REGISTER_SERVICE(mesh_registry)
        } // namespace mesh
    } // namespace render


} // namespace trishul

#endif //CURSEOFTHESEA_SERVICES_H
