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
#ifndef CURSEOFTHESEA_SERVICE_LOCATOR_H
#define CURSEOFTHESEA_SERVICE_LOCATOR_H

#include <memory>
#include <mutex>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include "trishul/core/engine_assert.h"
#include "trishul/engine_api.h"

namespace trishul
{
    namespace service_detail
    {
        template<typename T>
        void destroy(void* p) noexcept
        {
            delete static_cast<T*>(p);
        }

        struct service_deleter
        {
            void (*fn)(void*) noexcept { nullptr };
            void operator()(void* p) const noexcept { if (fn) fn(p); }
        };

        //~ locator owns each service no refcount
        using service_ptr = std::unique_ptr<void, service_deleter>;
        using service_map = std::unordered_map<std::string, service_ptr>;

        TRISHUL_API service_map& services     ();
        TRISHUL_API service_map& null_services();
        TRISHUL_API std::mutex&  locator_mutex();

        template<typename T>
        __forceinline std::string type_key()
        {
            return std::string(typeid(T).name());
        }
    } // namespace service_detail

    class service_locator
    {
    public:
        //~ hand ownership to the locator
        template<typename T>
        static void provide(std::unique_ptr<T> service)
        {
            std::lock_guard lock(service_detail::locator_mutex());
            service_detail::services()[service_detail::type_key<T>()] =
                service_detail::service_ptr(
                    service.release(),
                    service_detail::service_deleter{ &service_detail::destroy<T> });
        }

        template<typename T>
        static void provide_null(std::unique_ptr<T> service)
        {
            std::lock_guard lock(service_detail::locator_mutex());
            service_detail::null_services()[service_detail::type_key<T>()] =
                service_detail::service_ptr(
                    service.release(),
                    service_detail::service_deleter{ &service_detail::destroy<T> });
        }

        //~ non owning view may be null
        template<typename T>
        static T* try_get()
        {
            std::lock_guard lock(service_detail::locator_mutex());
            const auto key = service_detail::type_key<T>();

            if (auto it = service_detail::services().find(key);
                it != service_detail::services().end())
                return static_cast<T*>(it->second.get());

            if (auto it = service_detail::null_services().find(key);
                it != service_detail::null_services().end())
                return static_cast<T*>(it->second.get());

            return nullptr;
        }

        //~ non owning view asserts present
        template<typename T>
        static T* get()
        {
            T* service = try_get<T>();
            ENGINE_VERIFY_MSG(service != nullptr,
                "no service registered for type {}", service_detail::type_key<T>().c_str());
            return service;
        }

        template<typename T>
        static bool has()
        {
            std::lock_guard lock(service_detail::locator_mutex());
            return service_detail::services().contains(service_detail::type_key<T>());
        }

        //~ reclaim ownership and drop from the locator
        template<typename T>
        static std::unique_ptr<T> release()
        {
            std::lock_guard lock(service_detail::locator_mutex());
            auto& map = service_detail::services();

            if (auto it = map.find(service_detail::type_key<T>()); it != map.end())
            {
                std::unique_ptr<T> owned(static_cast<T*>(it->second.release()));
                map.erase(it);
                return owned;
            }
            return nullptr;
        }

        template<typename T>
        static void remove()
        {
            std::lock_guard lock(service_detail::locator_mutex());
            service_detail::services().erase(service_detail::type_key<T>());
        }

        template<typename T>
        static void remove_null()
        {
            std::lock_guard lock(service_detail::locator_mutex());
            service_detail::null_services().erase(service_detail::type_key<T>());
        }

        static void clear()
        {
            std::lock_guard lock(service_detail::locator_mutex());
            service_detail::services().clear();
            service_detail::null_services().clear();
        }

        template<typename T>
        class scope
        {
        public:
            explicit scope(std::unique_ptr<T> service)
                : previous_(service_locator::release<T>())
            {
                service_locator::provide<T>(std::move(service));
            }

            ~scope()
            {
                if (previous_)
                    service_locator::provide<T>(std::move(previous_));
                else
                    service_locator::remove<T>();
            }

            scope           (const scope&) = delete;
            scope& operator= (const scope&) = delete;

        private:
            std::unique_ptr<T> previous_;
        };
    };
} // namespace trishul

#define REGISTER_SERVICE(T)namespace trishul{\
inline const bool _trishul_service_registered_##T = []{\
::trishul::service_locator::provide<T>(std::make_unique<T>());\
return true;\
}();}\

#endif //CURSEOFTHESEA_SERVICE_LOCATOR_H
