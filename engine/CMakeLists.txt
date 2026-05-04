add_library(engine SHARED
        src/engine.cpp
        src/utils/timer.cpp
        src/service_locator.cpp
)

target_include_directories(engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(engine PUBLIC cxx_std_23)
include(GenerateExportHeader)
generate_export_header(engine
        BASE_NAME COTS
        EXPORT_MACRO_NAME COTS_API
        EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/engine/cots_api.h
)

set_target_properties(engine PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
)

# fetch packages
include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG 79524dd
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(engine PUBLIC spdlog::spdlog)
