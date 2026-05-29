add_executable(cots WIN32
        src/game.cpp
        src/agility_sdk.cpp
        src/game_layer.cpp
        app.rc
)

cots_apply_compile_flags(cots)

target_include_directories(cots PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cots PRIVATE gameplay)
target_link_libraries(cots PRIVATE trishul_engine)

target_compile_features(cots PRIVATE cxx_std_23)
target_compile_definitions(cots PRIVATE
        _UNICODE UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

target_compile_definitions(cots PRIVATE COTS_AGILITY_SDK_VERSION=616)

set_target_properties(cots PROPERTIES PREFIX "")

add_custom_command(TARGET cots POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:gameplay>
        $<TARGET_FILE:trishul_engine>
        $<TARGET_FILE_DIR:cots>
        COMMENT "Copying gameplay DLL"
)

add_custom_command(TARGET cots POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:trishul_engine>
        $<TARGET_FILE_DIR:cots>
        COMMENT "Copying engine DLL"
)

add_custom_command(TARGET cots POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FMOD::Core>
        $<TARGET_FILE_DIR:cots>
        COMMENT "Copying FMOD Core DLL next to the cots exe"
)

#~ deco assets icons next to the exe so runtime file loads resolve
add_custom_command(TARGET cots POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/deco
        $<TARGET_FILE_DIR:cots>/deco
        COMMENT "Copying deco assets next to the cots exe"
)
