// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_INTERFACE_SINGLETON_H
#define CURSEOFTHESEA_INTERFACE_SINGLETON_H
#include <type_traits>

namespace cots::interface
{
    template<typename T>
    class singleton
    {
    protected:
         singleton() = default;
        ~singleton() = default;

    public:
        static T& instance() noexcept
        {
            static T s_instance;
            return instance;
        }

        singleton(const singleton&)            = delete;
        singleton(singleton&&)                 = delete;
        singleton& operator=(const singleton&) = delete;
        singleton& operator=(singleton&&)      = delete;
    };
};

#define COTS_SINGLETON(CLASS_NAME)\
friend class ::cots::interface::singleton<CLASS_NAME>;\
public:\
CLASS_NAME(const CLASS_NAME&)            = delete;\
CLASS_NAME(CLASS_NAME&&)                 = delete;\
CLASS_NAME& operator=(const CLASS_NAME&) = delete;\
CLASS_NAME& operator=(CLASS_NAME&&)      = delete;\
private:\

#endif //CURSEOFTHESEA_INTERFACE_SINGLETON_H
