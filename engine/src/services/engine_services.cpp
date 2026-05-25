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
#include "engine/system/service_registry.h"
#include "engine/engine.h"

namespace
{
    void get_fps_info(std::uint32_t& mt_fps, std::uint32_t& rt_fps)
    {
        const auto [mt, rt] = cots::engine::instance().get_fps_stats();
        mt_fps = mt;
        rt_fps = rt;
    }

    void set_target_fps(const std::uint32_t& mt_fps)
    {
        cots::engine::instance().set_target_fps(mt_fps);
    }

    void install(cots::module::services& services)
    {
        services.engine.get_fps_info   = &get_fps_info;
        services.engine.set_target_fps = &set_target_fps;
    }
} // namespace anonymous

COTS_INSTALL_SERVICES(install)
