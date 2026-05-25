// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_COTS_CONFIG_H
#define CURSEOFTHESEA_COTS_CONFIG_H

#include <cstdint>

//~ device dump information
#ifndef COTS_DRED_ENABLED
    #define COTS_DRED_ENABLED (!COTS_PRODUCTION)
#endif

//~ Build Kinds
#ifndef COTS_DEBUG
    #define COTS_DEBUG 0
#endif

#ifndef COTS_RELEASE
    #define COTS_RELEASE 0
#endif

#ifndef COTS_PRODUCTION
    #define COTS_PRODUCTION 0
#endif

#if (COTS_DEBUG + COTS_RELEASE + COTS_PRODUCTION) != 1
    #error "Exactly one of COTS_DEBUG / COTS_RELEASE / COTS_PRODUCTION must be 1"
#endif

#if COTS_DEBUG
    #define COTS_BUILD_NAME "Debug"
#elif COTS_RELEASE
    #define COTS_BUILD_NAME "Release"
#else
    #define COTS_BUILD_NAME "Production"
#endif

//~ Hot reload behaviour flags
#ifndef COTS_HOT_RELOAD
    #define COTS_HOT_RELOAD (!COTS_PRODUCTION)
#endif

// CRT debug runtime
#ifndef COTS_DEBUG_RUNTIME
    #define COTS_DEBUG_RUNTIME COTS_DEBUG
#endif

#ifndef COTS_TRACY_ENABLED
    #define COTS_TRACY_ENABLED 0
#endif
#ifndef COTS_PROFILER_LEVEL
    #define COTS_PROFILER_LEVEL 0
#endif

#define COTS_HAS_PROFILER (COTS_PROFILER_LEVEL > 0)

#endif //CURSEOFTHESEA_COTS_CONFIG_H
