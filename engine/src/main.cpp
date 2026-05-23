// Created by Niffoxic (Harsh Dubey)
#include "engine/engine.h"
#include "engine/core/cots_assert.h"
#include "engine/system/game_host.h"
#include "engine/system/service_registry.h"

#include <cots/engine_services.h>
#include <cots/cots_config.h>
#include <spdlog/spdlog.h>

#include "engine/audio/audio_system.h"
#include "engine/graphics/render.h"
#include "engine/system/feature_locator.h"
#include <timeapi.h>

int main()
{
#if defined(COTS_DEBUG_RUNTIME)
    cots::init_debug_runtime();
#endif

    timeBeginPeriod(1);
    cots::engine engine{};

    if (not engine.init())
    {
        spdlog::error("Failed to initialize engine");
        return 1;
    }

    cots::module::services services{};
    cots::services::install_all(services);

    cots::game::host host{};
    if (not host.initialize(services))
    {
        spdlog::error("engine: Failed to initialize game");
        return 1;
    }

    spdlog::info("engine: host initialized");
    std::uint32_t reload_check_counter = 0u;

    while (not engine.should_close())
    {
#if defined(COTS_HOT_RELOAD)
        if (++reload_check_counter >= 10) //~ cheap poll per 10 frames
        {
            reload_check_counter = 0u;
            host.poll_for_reload();
        }
#endif
        engine.tick();
        host.update(engine.delta_time());
    }

    spdlog::info("engine: shutting down");
    host.deinitialize();
    timeEndPeriod(1);
    return 0;
}
