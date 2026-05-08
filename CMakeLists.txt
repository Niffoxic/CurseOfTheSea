cmake_minimum_required(VERSION 4.2)
project(CurseOfTheSea)

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
option(COTS_USE_TRACY "Enable Tracy profiler integration" OFF)

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

# rest setup
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(shared)
add_subdirectory(engine)
add_subdirectory(game)

file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/assets/*")

add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:engine>/assets
        COMMENT "Copying assets next to engine.exe"
)

add_dependencies(engine copy_assets)
