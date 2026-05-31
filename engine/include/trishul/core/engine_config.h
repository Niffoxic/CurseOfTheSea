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
#ifndef CURSEOFTHESEA_ENGINE_CONFIG_H
#define CURSEOFTHESEA_ENGINE_CONFIG_H


#include <cstdint>

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

#if defined(COTS_DEBUG)
    #define COTS_BUILD_NAME "Debug"
#elif defined(COTS_RELEASE) || defined(COTS_RELWITHDEBINFO)
    #define COTS_BUILD_NAME "Release"
#else
    #define COTS_BUILD_NAME "Production"
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

#ifndef COTS_DRED_ENABLED
    #define COTS_DRED_ENABLED (!COTS_PRODUCTION)
#endif

#ifndef COTS_GPU_VALIDATION_ENABLED
    #define COTS_GPU_VALIDATION_ENABLED 0
#endif

#define COTS_HAS_PROFILER (COTS_PROFILER_LEVEL > 0)

namespace trishul::config
{
    static constexpr std::uint32_t INVALID_INDEX = ~0u;
    static constexpr std::uint32_t SWAPCHAIN_BUFFER_COUNT = 3u;

    static constexpr std::uint32_t MAX_LIGHTS              = 32u;
    static constexpr std::uint32_t MAX_INSTANCES_PER_FRAME = 1024u;

    static constexpr std::uint32_t SHADOW_CASCADE_COUNT    = 3u;
    static constexpr std::uint32_t CUBE_FACE_COUNT         = 6u;

    static constexpr std::uint32_t MAX_SPOT_SHADOWS        = 8u;
    static constexpr std::uint32_t MAX_POINT_SHADOWS       = 4u;

    static constexpr std::uint32_t MAX_FOG_VOLUMES         = 16u;

    static constexpr std::uint32_t SUN_SHADOW_RESOLUTION   = 2048u;
    static constexpr std::uint32_t SPOT_SHADOW_RESOLUTION  = 1024u;
    static constexpr std::uint32_t POINT_SHADOW_RESOLUTION = 512u;

    static constexpr std::uint32_t RENDER_SCENE_SNAPSHOT = 3u;
    static constexpr std::uint32_t MAX_BACKBUFFER_COUNT  = 3u;

    constexpr static std::uint32_t FRAME_COUNT        = 3u;  //~ max frames in flight
    constexpr static std::uint32_t MAX_SUBMIT_LISTS   = 8u; //~ max flight buffer


} // namespace trishul config

#endif //CURSEOFTHESEA_ENGINE_CONFIG_H
