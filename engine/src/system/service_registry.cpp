// Created by Niffoxic (Harsh Dubey)
#include "engine/system/service_registry.h"
#include <array>
#include <cstddef>

namespace cots::services
{
    namespace
    {
        std::array<installer_fn, max_services> g_installers{};
        std::size_t g_installer_count = 0;
    } // anonymous

    installer::installer(const installer_fn fn) noexcept
    {
        if (g_installer_count < max_services)
            g_installers[g_installer_count++] = fn;
    }

    void install_all(module::services &out)
    {
        for (size_t i = 0; i < g_installer_count; ++i)
        {
            g_installers[i](out);
        }
    }
} // namespace cots::services
