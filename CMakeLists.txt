cmake_minimum_required(VERSION 4.2)
project(CurseOfTheSea)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(engine)
add_subdirectory(game)

file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/assets/*")

add_custom_command(
        OUTPUT  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.assets_stamp
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
        COMMAND ${CMAKE_COMMAND} -E touch
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.assets_stamp
        DEPENDS ${ASSET_FILES}
        COMMENT "Copying assets to bin/assets"
)

add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:game>/assets
        COMMENT "Copying assets next to the executable"
)

add_dependencies(game copy_assets)
