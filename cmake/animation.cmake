include(FetchContent)

function(cots_fetch_ozz)
    if(TARGET ozz_animation)
        return()
    endif()

    set(ozz_build_samples         OFF CACHE BOOL "" FORCE)
    set(ozz_build_howtos          OFF CACHE BOOL "" FORCE)
    set(ozz_build_tests           OFF CACHE BOOL "" FORCE)
    set(ozz_build_fbx             OFF CACHE BOOL "" FORCE)
    set(ozz_build_tools           OFF CACHE BOOL "" FORCE)
    set(ozz_build_data            OFF CACHE BOOL "" FORCE)
    set(ozz_build_gltf            OFF CACHE BOOL "" FORCE)
    set(ozz_build_postfix         OFF CACHE BOOL "" FORCE)
    set(ozz_build_msvc_rt_dll     ON  CACHE BOOL "" FORCE)

    FetchContent_Declare(
            ozz_animation
            GIT_REPOSITORY https://github.com/guillaumeblanc/ozz-animation.git
            GIT_TAG 0.16.0
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(ozz_animation)
endfunction()

function(cots_link_ozz target visibility)
    cots_fetch_ozz()
    target_link_libraries(${target} ${visibility}
            ozz_base
            ozz_animation
            ozz_animation_offline
    )
endfunction()

function(cots_fetch_ufbx)
    if(TARGET ufbx)
        FetchContent_GetProperties(ufbx)
        return()
    endif()

    FetchContent_Declare(
            ufbx
            GIT_REPOSITORY https://github.com/ufbx/ufbx.git
            GIT_TAG v0.20.0
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(ufbx)

    if(NOT TARGET ufbx)
        add_library(ufbx STATIC ${ufbx_SOURCE_DIR}/ufbx.c)
        target_include_directories(ufbx PUBLIC ${ufbx_SOURCE_DIR})
        set_target_properties(ufbx PROPERTIES
                C_STANDARD          11
                C_STANDARD_REQUIRED ON
                POSITION_INDEPENDENT_CODE ON
        )
    endif()
endfunction()

function(cots_link_ufbx target visibility)
    cots_fetch_ufbx()
    target_link_libraries(${target} ${visibility} ufbx)
endfunction()
