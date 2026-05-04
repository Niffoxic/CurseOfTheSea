// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_SERVICE_LOCATOR_H
#define CURSEOFTHESEA_SERVICE_LOCATOR_H

#include <memory>
#include <mutex>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include "core/cots_assert.h"

namespace cots
{
    /**
     * @class service_locator
     *
     * @brief A class that implements the Service Locator pattern to manage dependencies and provide access to various services.
     *
     * The service_locator class is responsible for registering and retrieving service instances. It facilitates decoupling
     * between service consumers and the actual implementations of those services. This pattern can be used as an alternative
     * to dependency injection to centralize and abstract dependency management.
     *
     * This class typically maintains a registry of service instances and their corresponding types or identifiers.
     * Consumers can retrieve services from the registry by their type or key.
     *
     * Features include:
     * - Registering a service instance.
     * - Retrieving a registered service instance.
     * - Checking whether a specific service is registered.
     * - Removing a specific service from the registry.
     *
     * Note:
     * - The service_locator pattern can lead to tight coupling and should be used sparingly in large-scale, maintainable systems.
     */
    class service_locator
    {
    public:
        //~ Register a service instance for type T
        template<typename T>
        static void provide(std::shared_ptr<T> service)
        {
            std::lock_guard lock(mutex_);
            const auto type_index = std::type_index(typeid(T));
            services_[type_index] = std::move(service);
        }

        // Fallback to the default
        template<typename T>
        static void provide_null(std::shared_ptr<T> service)
        {
            std::lock_guard lock(mutex_);
            const auto type_index = std::type_index(typeid(T));
            nullServices_[type_index] = std::move(service);
        }

        template<typename T>
        static T* resolve()
        {
            std::lock_guard lock(mutex_);
            const auto type_index = std::type_index(typeid(T));

            if (services_.contains(type_index))
            {
                return static_cast<T*>(services_[type_index].get());
            }
            if (nullServices_.contains(type_index))
            {
                return static_cast<T*>(nullServices_[type_index].get());
            }
            COTS_FAIL_MSG("No service registered for type: {}", type_index.name());
            throw std::runtime_error(
                std::string("ServiceLocator: no service registered for ") + type_index.name());
        }

        template<typename T>
        static bool has()
        {
            std::lock_guard lock(mutex_);
            const auto type_index = std::type_index(typeid(T));
            return services_.contains(type_index);
        }

        template<typename T>
        static void remove()
        {
            std::lock_guard lock(mutex_);
            const auto type_index = std::type_index(typeid(T));
            services_.erase(type_index);
        }

        template<typename T>
        static void remove_null()
        {
            std::lock_guard lock(mutex_);
            const auto type_index = std::type_index(typeid(T));
            nullServices_.erase(type_index);
        }

        static void clear() //~ dont think ever gonna use it xD
        {
            std::lock_guard lock(mutex_);
            services_.clear();
            nullServices_.clear();
        }

        template<typename T>
        class scope
        {
        public:
            explicit scope(std::shared_ptr<T> service)
            {
                previous_ = service_locator::has<T>() ? service_locator::resolve<T>() : nullptr;
                service_locator::provide<T>(std::move(service));
            }

            ~scope()
            {
                if (previous_)
                    service_locator::provide<T>(std::move(previous_));
                else
                    service_locator::remove<T>();
            }

            scope           (const scope &) = delete;
            scope &operator=(const scope &) = delete;

        private:
            std::shared_ptr<T> previous_;
        };

    private:
        static std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
        static std::unordered_map<std::type_index, std::shared_ptr<void>> nullServices_;
        static std::mutex mutex_;
    };
} // namespace cots

#endif //CURSEOFTHESEA_SERVICE_LOCATOR_H
