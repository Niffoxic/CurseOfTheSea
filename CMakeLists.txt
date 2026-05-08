cmake_minimum_required(VERSION 4.2)
project(CurseOfTheSea)

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
