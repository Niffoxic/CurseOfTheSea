// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_PROFILER_H
#define CURSEOFTHESEA_PROFILER_H

#include <cots/cots_config.h>

#define COTS_PROFILE_CONCAT_(a, b) a##b
#define COTS_PROFILE_CONCAT(a, b)  COTS_PROFILE_CONCAT_(a, b)

#if COTS_TRACY_ENABLED
    #include <tracy/Tracy.hpp>

    #define COTS_PROFILE_SCOPE(name)       ZoneScopedN(name)
    #define COTS_PROFILE_FRAME_MARK(name)  FrameMarkNamed(name)
    #define COTS_PROFILE_THREAD_NAME(name) tracy::SetThreadName(name)
#else
    #define COTS_PROFILE_SCOPE(name)       ((void)0)
    #define COTS_PROFILE_FRAME_MARK(name)  ((void)0)
    #define COTS_PROFILE_THREAD_NAME(name) ((void)0)
#endif

#endif
