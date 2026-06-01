add_library(trishul_engine SHARED
        src/engine.cpp
        src/platform/inputs/mouse_component.cpp
        src/platform/inputs/keyboard_component.cpp
        src/platform/platform_windows.cpp
        src/service_locator.cpp
        src/utils/timer.cpp
        src/renderer/render.cpp
        src/renderer/hardware/device.cpp
        src/core/exception/dx_exception.cpp
        src/renderer/hardware/fence.cpp
        src/renderer/hardware/swapchain.cpp
        src/renderer/hardware/descriptor_heap.cpp
        src/renderer/hardware/queue_timeline.cpp
        src/renderer/hardware/depth_target.cpp
        src/renderer/hardware/upload_arena.cpp
        src/renderer/hardware/deferred_releaser.cpp
        src/renderer/hardware/texture_manager.cpp
        src/renderer/hardware/buffer_manager.cpp
        src/renderer/shaders/shader_compiler.cpp
        src/renderer/shaders/shader_cache.cpp
        src/renderer/shaders/root_sig_builder.cpp
        src/renderer/shaders/storage/binary_storage.cpp
        src/renderer/shaders/storage/json_storage.cpp
        src/renderer/cull/frustum.cpp
        src/renderer/cull/bvh.cpp
        src/renderer/materials/material_registry.cpp
        src/renderer/materials/material_root_sig.cpp
        src/renderer/materials/pso_cache.cpp
        src/renderer/particles/particle_system.cpp
        src/renderer/textures/texture_baker.cpp
        src/renderer/textures/texture_cache.cpp
        src/renderer/utils/image_decode.cpp
        src/renderer/graph/resource_registry.cpp
        src/renderer/hardware/command_context.cpp
)

cots_apply_compile_flags(trishul_engine)

set_target_properties(trishul_engine PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
set_target_properties(trishul_engine PROPERTIES PREFIX "")

target_include_directories(trishul_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(trishul_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(trishul_engine PRIVATE
        _UNICODE UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        BUILDING_ENGINE
)

target_compile_features(trishul_engine PUBLIC cxx_std_23)

include(dependencies)
cots_add_general_dependencies(trishul_engine)

include(d3d12)
cots_add_d3d12(trishul_engine)

#~ moodycamel concurrentqueue
include(workers)
cots_fetch_concurrentqueue()
target_link_libraries(trishul_engine PRIVATE concurrentqueue)

# audio
target_link_libraries(trishul_engine PRIVATE FMOD::Core)

# physics jolt
include(jolt)
cots_link_jolt(trishul_engine PRIVATE)

# effects Effekseer
include(effekseer)
cots_link_effekseer(trishul_engine PRIVATE)

option(COTS_ENABLE_DEBUG_DRAW
        "Enable debug drawable runtime body"
        ON)

target_compile_definitions(trishul_engine PUBLIC
        "$<$<AND:$<BOOL:${COTS_ENABLE_DEBUG_DRAW}>,$<NOT:$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>>:COTS_DEBUG_DRAW=1>"
)
