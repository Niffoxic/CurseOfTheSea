add_executable(engine
        src/main.cpp
        src/engine.cpp
        src/utils/timer.cpp
        src/platform/platform_windows.cpp
        src/system/game_host.cpp
        src/system/service_registry.cpp
        src/system/log_services.cpp
        src/system/window_services.cpp
        src/platform/components/keyboard_component.cpp
        src/platform/components/mouse_component.cpp
        src/audio/audio_system.cpp
        src/audio/backend/null_backend.cpp
        src/audio/backend/fmod_backend.cpp
        src/system/audio_services.cpp
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

# logger
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG 79524dd
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(engine PRIVATE spdlog::spdlog)

# ecs and event dispatcher
FetchContent_Declare(
        EnTT
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.16.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(EnTT)
target_link_libraries(engine PRIVATE EnTT::EnTT)

# audio
target_link_libraries(engine PRIVATE FMOD::Core)

add_custom_command(TARGET engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FMOD::Core>
        $<TARGET_FILE_DIR:engine>
        COMMENT "Copying FMOD Core DLL"
)
