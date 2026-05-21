// Created by Niffoxic (Harsh Dubey)
#include "engine/system/service_registry.h"
#include <spdlog/spdlog.h>

namespace
{
    void log_info (const char* msg) { spdlog::info ("[game] {}", msg); }
    void log_warn (const char* msg) { spdlog::warn ("[game] {}", msg); }
    void log_error(const char* msg) { spdlog::error("[game] {}", msg); }

    void install(cots::module::services& s)
    {
        s.log.info  = &log_info;
        s.log.warn  = &log_warn;
        s.log.error = &log_error;
    }
}

COTS_INSTALL_SERVICES(install)
