// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_PROFILER_H
#define CURSEOFTHESEA_PROFILER_H

#include <cstdint>

#define COTS_PROFILE_CONCAT_(a, b) a##b
#define COTS_PROFILE_CONCAT(a, b)  COTS_PROFILE_CONCAT_(a, b)

#if defined(USE_PIX)
    #include <WinPixEventRuntime/pix3.h>

    #define COTS_PROFILE_BEGIN(name, color) PIXBeginEvent(color, name)
    #define COTS_PROFILE_END()              PIXEndEvent()
    #define COTS_PROFILE_MARKER(name, color)PIXSetMarker(color, name)

    #define COTS_GPU_PROFILE_BEGIN(cmd, name, color) PIXBeginEvent(cmd, color, name)
    #define COTS_GPU_PROFILE_END(cmd)                PIXEndEvent(cmd)
    #define COTS_GPU_PROFILE_MARKER(cmd, name, color)PIXSetMarker(cmd, color, name)
#else
    #define COTS_PROFILE_BEGIN(name, color)            ((void)0)
    #define COTS_PROFILE_END()                         ((void)0)
    #define COTS_PROFILE_MARKER(name, color)           ((void)0)
    #define COTS_GPU_PROFILE_BEGIN(cmd, name, color)   ((void)0)
    #define COTS_GPU_PROFILE_END(cmd)                  ((void)0)
    #define COTS_GPU_PROFILE_MARKER(cmd, name, color)  ((void)0)
#endif

namespace cots::profile
{
    struct scope
    {
        scope(const char* name, const std::uint64_t color)
        {
            COTS_PROFILE_BEGIN(name, color);
        }

        ~scope()
        {
            COTS_PROFILE_END();
        }
    };
}

#define COTS_PROFILE_SCOPE(name) \
::cots::profile::scope COTS_PROFILE_CONCAT(_cots_prof_scope_, __LINE__)(name, 0xFF888888)

#endif
