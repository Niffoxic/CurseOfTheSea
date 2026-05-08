// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SERVICE_REGISTRY_H
#define CURSEOFTHESEA_SERVICE_REGISTRY_H

#include <cstddef>
#include <cots/engine_services.h>

namespace cots::services
{
    static constexpr std::size_t max_services = 128;

    using installer_fn = void(*)(module::services&);

    struct installer
    {
        explicit installer(installer_fn fn) noexcept;
    };

    void install_all(module::services& out);
} // namespace cots::services

#define COTS_INSTALL_SERVICES(fn)           \
    namespace {                             \
    static const ::cots::services::installer\
    _cots_services_installer_##fn(&fn);     \
}

#endif //CURSEOFTHESEA_SERVICE_REGISTRY_H
