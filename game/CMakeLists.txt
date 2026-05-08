file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(game SHARED ${GAME_SOURCES})

target_link_libraries(game PRIVATE cots_shared)

target_compile_features(game PRIVATE cxx_std_23)
target_compile_definitions(game PRIVATE
        _UNICODE UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

set_target_properties(game PROPERTIES PREFIX "")

string(TIMESTAMP COTS_GAME_BUILD_TS "%Y%m%d_%H%M%S")
set_target_properties(game PROPERTIES PDB_NAME "game_${COTS_GAME_BUILD_TS}")
