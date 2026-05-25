add_executable(engine
        src/main.cpp
        src/engine.cpp
        src/utils/timer.cpp
        src/platform/platform_windows.cpp
        src/system/game_host.cpp
        src/system/service_registry.cpp
        src/services/log_services.cpp
        src/services/window_services.cpp
        src/services/editor_services.cpp
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
        src/graphics/meshes/mesh_registry.cpp
        src/graphics/passes/mesh_pass.cpp
        src/graphics/resource/depth_target.cpp
        src/graphics/graph/resource_registry.cpp
        src/graphics/graph/render_graph.cpp
        src/graphics/hardware/descriptor_heap.cpp
        src/graphics/hardware/texture_manager.cpp
        src/graphics/utils/image_decode.cpp
        src/graphics/textures/texture_baker.cpp
        src/graphics/textures/texture_cache.cpp
        src/graphics/shaders/root_sig_builder.cpp
        src/graphics/meshes/gltf_importer.cpp
        src/graphics/meshes/mesh_baker.cpp
        src/graphics/meshes/mesh_cache.cpp
        src/services/engine_services.cpp
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

if(COTS_USE_EDITOR)
    target_link_libraries(engine PRIVATE editor)
endif()

include(cots_dependencies)
cots_add_general_dependencies(engine)

include(cots_d3d12)
cots_add_d3d12(engine)

# audio
target_link_libraries(engine PRIVATE FMOD::Core)

add_custom_command(TARGET engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FMOD::Core>
        $<TARGET_FILE_DIR:engine>
        COMMENT "Copying FMOD Core DLL"
)
