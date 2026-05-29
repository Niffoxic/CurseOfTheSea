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
#ifndef CURSEOFTHESEA_SINGLETON_H
#define CURSEOFTHESEA_SINGLETON_H
#include <type_traits>

namespace trishul::interfaces
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
            return s_instance;
        }

        singleton(const singleton&)            = delete;
        singleton(singleton&&)                 = delete;
        singleton& operator=(const singleton&) = delete;
        singleton& operator=(singleton&&)      = delete;
    };
} // namespace trishul::interfaces

#define ENGINE_SINGLETON(CLASS_NAME)\
friend class ::trishul::interfaces::singleton<CLASS_NAME>;\
public:\
CLASS_NAME(const CLASS_NAME&)            = delete;\
CLASS_NAME(CLASS_NAME&&)                 = delete;\
CLASS_NAME& operator=(const CLASS_NAME&) = delete;\
CLASS_NAME& operator=(CLASS_NAME&&)      = delete;\
private:\

#endif //CURSEOFTHESEA_SINGLETON_H
