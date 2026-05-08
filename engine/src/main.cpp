// Created by Niffoxic (Harsh Dubey)
#include "engine/engine.h"
#include "engine/core/cots_assert.h"
#include "../include/engine/system/game_host.h"

#include <cots/engine_services.h>
#include <cots/cots_config.h>

#include <spdlog/spdlog.h>

#include "engine/system/service_registry.h"

namespace //~ tests
{
    void log_info (const char* msg) { spdlog::info ("[game] {}", msg); }
    void log_warn (const char* msg) { spdlog::warn ("[game] {}", msg); }
    void log_error(const char* msg) { spdlog::error("[game] {}", msg); }
}

int main()
{
#if defined(COTS_DEBUG_RUNTIME)
    cots::init_debug_runtime();
#endif

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
    return 0;
}
