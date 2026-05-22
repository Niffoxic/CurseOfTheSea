add_executable(engine
        src/main.cpp
        src/engine.cpp
        src/utils/timer.cpp
        src/platform/platform_windows.cpp
        src/system/game_host.cpp
        src/system/service_registry.cpp
        src/services/log_services.cpp
        src/services/window_services.cpp
        src/platform/components/keyboard_component.cpp
        src/platform/components/mouse_component.cpp
        src/audio/audio_system.cpp
        src/audio/backend/null_backend.cpp
        src/audio/backend/fmod_backend.cpp
        src/services/audio_services.cpp
        src/services/input_services.cpp
        src/agility_sdk.cpp
        src/graphics/render.cpp
        src/graphics/hardware/device.cpp
        src/graphics/utils/exception.cpp
        src/graphics/hardware/swapchain.cpp
        src/graphics/hardware/fence.cpp
        src/graphics/hardware/command_context.cpp
        src/services/render_services.cpp
        src/graphics/passes/clear_pass.cpp
        src/graphics/passes/present_pass.cpp
        src/graphics/shaders/shader_cache.cpp
        src/graphics/shaders/shader_compiler.cpp
        src/graphics/shaders/storage/binary_storage.cpp
        src/graphics/shaders/storage/json_storage.cpp
        src/graphics/hardware/buffer_manager.cpp
        src/graphics/passes/triangle_pass.cpp
        src/graphics/resource/constant_ring.cpp
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

# dx12 agility sdk
set(AGILITY_SDK_VERSION "1.616.0")
set(AGILITY_SDK_BUILD_VERSION 616)

FetchContent_Declare(
        agility_sdk
        URL "https://globalcdn.nuget.org/packages/microsoft.direct3d.d3d12.${AGILITY_SDK_VERSION}.nupkg"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(agility_sdk)

target_include_directories(engine PRIVATE
        ${agility_sdk_SOURCE_DIR}/build/native/include
)

target_compile_definitions(engine PRIVATE
        COTS_AGILITY_SDK_VERSION=${AGILITY_SDK_BUILD_VERSION}
)
target_link_libraries(engine PRIVATE d3d12 dxgi dxguid winmm)

add_custom_command(TARGET engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:engine>/D3D12

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${agility_sdk_SOURCE_DIR}/build/native/bin/x64/D3D12Core.dll
        ${agility_sdk_SOURCE_DIR}/build/native/bin/x64/d3d12SDKLayers.dll
        $<TARGET_FILE_DIR:engine>/D3D12

        COMMENT "Copying Agility SDK DLLs to D3D12/"
)

# WinPIX Event Runtime
set(WINPIX_VERSION "1.0.240308001")

FetchContent_Declare(
        winpix
        URL "https://globalcdn.nuget.org/packages/winpixeventruntime.${WINPIX_VERSION}.nupkg"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(winpix)

target_include_directories(engine PRIVATE
        ${winpix_SOURCE_DIR}/Include
)

target_link_libraries(engine PRIVATE
        ${winpix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.lib
)

target_compile_definitions(engine PRIVATE
        $<$<CONFIG:Debug>:USE_PIX=1>
        $<$<CONFIG:Release>:USE_PIX=1>
)

add_custom_command(TARGET engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${winpix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll
        $<TARGET_FILE_DIR:engine>
        COMMENT "Copying WinPixEventRuntime.dll"
)

# audio
target_link_libraries(engine PRIVATE FMOD::Core)

add_custom_command(TARGET engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FMOD::Core>
        $<TARGET_FILE_DIR:engine>
        COMMENT "Copying FMOD Core DLL"
)

# DirectX Shader Compiler
set(DXC_VERSION "1.9.2602.17")
FetchContent_Declare(
        dxc
        URL "https://globalcdn.nuget.org/packages/microsoft.direct3d.dxc.${DXC_VERSION}.nupkg"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(dxc)

target_include_directories(engine PRIVATE
        ${dxc_SOURCE_DIR}/build/native/include
)
target_link_libraries(engine PRIVATE
        ${dxc_SOURCE_DIR}/build/native/lib/x64/dxcompiler.lib
)
# dxcompiler.dll compiles and dxil.dll signs
add_custom_command(TARGET engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${dxc_SOURCE_DIR}/build/native/bin/x64/dxcompiler.dll
        ${dxc_SOURCE_DIR}/build/native/bin/x64/dxil.dll
        $<TARGET_FILE_DIR:engine>
        COMMENT "Copying DXC runtime DLLs"
)

# JSON
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)
target_link_libraries(engine PRIVATE nlohmann_json::nlohmann_json)

# D3D12 Memory Allocator
FetchContent_Declare(
        d3d12ma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git
        GIT_TAG v3.1.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(d3d12ma)
target_link_libraries(engine PRIVATE D3D12MemoryAllocator)
