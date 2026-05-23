include(cots_editor)
cots_fetch_imgui()

add_library(editor STATIC
        src/editor.cpp
        src/world.cpp
        src/passes/editor_pass.cpp
        src/panels/stats_panel.cpp
        src/panels/scene_panel.cpp
        src/panels/resource_panel.cpp
        src/commands/spawn_actor.cpp
        src/commands/delete_actor.cpp
        src/commands/load_mesh.cpp
        src/commands/load_texture.cpp
)

cots_apply_compile_flags(editor)

target_include_directories(editor PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/engine/include
)

target_link_libraries(editor PUBLIC
        cots_shared
        imgui::imgui
)

target_compile_features(editor PUBLIC cxx_std_23)

target_compile_definitions(editor PRIVATE
        _UNICODE UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

if(NOT TARGET spdlog::spdlog)
    include(FetchContent)
    FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG 79524dd
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(spdlog)
endif()
target_link_libraries(editor PRIVATE spdlog::spdlog)
target_link_libraries(editor PRIVATE d3d12 dxgi dxguid)
