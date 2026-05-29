add_library(gameplay SHARED
        src/application.cpp
        src/layer.cpp
        src/layer_stack.cpp
)

cots_apply_compile_flags(gameplay)
set_target_properties(gameplay PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
set_target_properties(gameplay PROPERTIES PREFIX "")

target_include_directories(gameplay PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(gameplay PUBLIC cxx_std_23)

target_compile_definitions(gameplay PRIVATE
        _UNICODE UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        BUILDING_GAMEPLAY
)

target_link_libraries(gameplay PUBLIC engine winmm)
