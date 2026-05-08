add_executable(engine
        src/main.cpp
        src/engine.cpp
        src/utils/timer.cpp
        src/platform/platform_windows.cpp
        src/system/game_host.cpp
        src/system/service_registry.cpp
        src/system/log_services.cpp
        src/system/window_services.cpp
)

cots_apply_compile_flags(engine)

target_include_directories(engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(engine PRIVATE
        _UNICODE UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

target_compile_features(engine PUBLIC cxx_std_23)

add_dependencies(engine game)
target_link_libraries(engine PRIVATE cots_shared)

# fetch packages
include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG 79524dd
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(engine PRIVATE  spdlog::spdlog)
