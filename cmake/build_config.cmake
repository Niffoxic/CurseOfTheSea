# custom build configurations
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_is_multi_config)
    foreach(_cfg RelWithDebInfo Production)
        if (NOT "${_cfg}" IN_LIST CMAKE_CONFIGURATION_TYPES)
            list(APPEND CMAKE_CONFIGURATION_TYPES ${_cfg})
        endif()
    endforeach()
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}"
            CACHE STRING "" FORCE)
endif()

# add release flags to the production
set(CMAKE_CXX_FLAGS_PRODUCTION            "${CMAKE_CXX_FLAGS_RELEASE}"            CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_PRODUCTION              "${CMAKE_C_FLAGS_RELEASE}"              CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PRODUCTION     "${CMAKE_EXE_LINKER_FLAGS_RELEASE}"     CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_PRODUCTION  "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}"  CACHE STRING "" FORCE)
mark_as_advanced(
        CMAKE_CXX_FLAGS_PRODUCTION CMAKE_C_FLAGS_PRODUCTION
        CMAKE_EXE_LINKER_FLAGS_PRODUCTION CMAKE_SHARED_LINKER_FLAGS_PRODUCTION
)

option(COTS_USE_TRACY "Enable Tracy profiler integration" ON)

option(COTS_TEXTURES_USE_BAKED_IN_DEBUG
        "Force the baked texture path even in Debug builds" ON)

#~ gpu based validation is opt in its brutally slow minutes to first frame and
#~ spams the startup banner on every device create flip it on only when chasing
#~ a descriptor or resource bug TODO: Find a better gpu validator if there is any
option(COTS_GPU_VALIDATION "Enable D3D12 GPU-based validation in debug builds" OFF)

set(COTS_PROFILER_LEVEL "0" CACHE STRING
        "Profiler verbosity: 0=off, 1=coarse, 2=medium, 3=verbose")
set_property(CACHE COTS_PROFILER_LEVEL PROPERTY STRINGS "0" "1" "2" "3")

# helper
function(cots_apply_compile_flags target)
    target_compile_definitions(${target} PRIVATE
            # Build config
            $<$<CONFIG:Debug>:COTS_DEBUG=1>
            $<$<CONFIG:Release>:COTS_RELEASE=1>
            $<$<CONFIG:RelWithDebInfo>:COTS_RELWITHDEBINFO=1>
            $<$<CONFIG:Production>:COTS_PRODUCTION=1>

            # Feature flags
            $<$<BOOL:${COTS_USE_TRACY}>:COTS_TRACY_ENABLED=1>
            $<$<BOOL:${COTS_TEXTURES_USE_BAKED_IN_DEBUG}>:COTS_TEXTURES_USE_BAKED_IN_DEBUG=1>
            $<$<BOOL:${COTS_GPU_VALIDATION}>:COTS_GPU_VALIDATION_ENABLED=1>
            COTS_PROFILER_LEVEL=${COTS_PROFILER_LEVEL}
    )
endfunction()
