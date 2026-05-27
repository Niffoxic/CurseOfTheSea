include(FetchContent)

function(cots_fetch_jolt)
    if(TARGET Jolt)
        return()
    endif()

    set(TARGET_HELLO_WORLD       OFF CACHE BOOL "" FORCE)
    set(TARGET_PERFORMANCE_TEST  OFF CACHE BOOL "" FORCE)
    set(TARGET_SAMPLES           OFF CACHE BOOL "" FORCE)
    set(TARGET_UNIT_TESTS        OFF CACHE BOOL "" FORCE)
    set(TARGET_VIEWER            OFF CACHE BOOL "" FORCE)

    set(ENABLE_OBJECT_STREAM     OFF CACHE BOOL "" FORCE)

    set(CROSS_PLATFORM_DETERMINISTIC OFF CACHE BOOL "" FORCE)

    set(USE_SSE4_1               ON  CACHE BOOL "" FORCE)
    set(USE_SSE4_2               ON  CACHE BOOL "" FORCE)
    set(USE_AVX                  ON  CACHE BOOL "" FORCE)
    set(USE_AVX2                 ON  CACHE BOOL "" FORCE)
    set(USE_LZCNT                ON  CACHE BOOL "" FORCE)
    set(USE_TZCNT                ON  CACHE BOOL "" FORCE)
    set(USE_F16C                 ON  CACHE BOOL "" FORCE)
    set(USE_FMADD                ON  CACHE BOOL "" FORCE)

    set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)

    set(OVERRIDE_CXX_FLAGS       OFF CACHE BOOL "" FORCE)

    set(DOUBLE_PRECISION         OFF CACHE BOOL "" FORCE)

    set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
    set(DEBUG_RENDERER_IN_DISTRIBUTION      OFF CACHE BOOL "" FORCE)

    set(PROFILER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
    set(PROFILER_IN_DISTRIBUTION      OFF CACHE BOOL "" FORCE)

    set(USE_ASSERTS              ON  CACHE BOOL "" FORCE)

    FetchContent_Declare(
            JoltPhysics
            GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
            GIT_TAG v5.2.0
            GIT_SHALLOW TRUE
            SOURCE_SUBDIR Build
    )
    FetchContent_MakeAvailable(JoltPhysics)
endfunction()

function(cots_link_jolt target visibility)
    cots_fetch_jolt()
    target_link_libraries(${target} ${visibility} Jolt)
endfunction()
