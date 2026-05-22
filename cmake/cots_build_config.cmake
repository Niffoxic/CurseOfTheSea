# custom build configurations
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_is_multi_config)
    if (NOT "Production" IN_LIST CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES Production)
        set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}"
            CACHE STRING "" FORCE)
    endif()
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

# its only for dev speed test with different implementation
option(COTS_USE_TRACY "Enable Tracy profiler integration" ON)

set(COTS_PROFILER_LEVEL "0" CACHE STRING
        "Profiler verbosity: 0=off, 1=coarse, 2=medium, 3=verbose")
set_property(CACHE COTS_PROFILER_LEVEL PROPERTY STRINGS "0" "1" "2" "3")

# helper
function(cots_apply_compile_flags target)
    target_compile_definitions(${target} PRIVATE
            # Build config
            $<$<CONFIG:Debug>:COTS_DEBUG=1>
            $<$<CONFIG:Release>:COTS_RELEASE=1>
            $<$<CONFIG:Production>:COTS_PRODUCTION=1>

            # Feature flags
            $<$<BOOL:${COTS_USE_TRACY}>:COTS_TRACY_ENABLED=1>
            COTS_PROFILER_LEVEL=${COTS_PROFILER_LEVEL}
    )
endfunction()
