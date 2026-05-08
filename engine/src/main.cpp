// Created by Niffoxic (Harsh Dubey)
#include "engine/engine.h"
#include "engine/core/cots_assert.h"
#include "game_host.h"

#include <cots/engine_services.h>
#include <spdlog/spdlog.h>

namespace //~ tests
{
    void log_info (const char* msg) { spdlog::info ("[game] {}", msg); }
    void log_warn (const char* msg) { spdlog::warn ("[game] {}", msg); }
    void log_error(const char* msg) { spdlog::error("[game] {}", msg); }
}

int main()
{
    cots::init_debug_runtime();
    cots::engine engine{};

    if (not engine.init())
    {
        spdlog::error("Failed to initialize engine");
        return 1;
    }

    cots::module::services services{};
    services.log_info  = &log_info;
    services.log_warn  = &log_warn;
    services.log_error = &log_error;

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
        if (++reload_check_counter >= 10) //~ cheap poll per 10 frames
        {
            reload_check_counter = 0u;
            host.pool_for_reload();
        }
        engine.tick();
        host.update(engine.delta_time());
    }

    spdlog::info("engine: shutting down");
    host.deinitialize();
    return 0;
}
