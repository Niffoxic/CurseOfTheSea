set(COTS_EFFEKSEER_REPOSITORY
        "https://github.com/effekseer/Effekseer.git"
        CACHE STRING "Effekseer git repository")

set(COTS_EFFEKSEER_GIT_TAG
        "1803"
        CACHE STRING "Effekseer git tag to checkout")

set(COTS_EFFEKSEER_VENDOR_DIR
        "${CMAKE_SOURCE_DIR}/vendor/effekseer"
        CACHE PATH "Local Effekseer source tree")

set(COTS_EFFEKSEER_BINARY_DIR
        "${CMAKE_BINARY_DIR}/_efk"
        CACHE PATH "Effekseer binary directory")

set(COTS_EFFEKSEER_AUTO_CLONE
        ON
        CACHE BOOL "Automatically clone Effekseer if missing")

set(COTS_EFFEKSEER_SYNC_TAG
        ON
        CACHE BOOL "Automatically checkout the configured Effekseer tag")

set(COTS_EFFEKSEER_UPDATE_SUBMODULES
        ON
        CACHE BOOL "Automatically update Effekseer submodules")


function(cots_git_fail step result output error)
    if(NOT "${result}" STREQUAL "0")
        message(FATAL_ERROR
                "Effekseer git step failed: ${step}\n"
                "Result: ${result}\n"
                "Output:\n${output}\n"
                "Error:\n${error}\n")
    endif()
endfunction()


function(cots_run_git step)
    execute_process(
            COMMAND "${GIT_EXECUTABLE}" ${ARGN}
            RESULT_VARIABLE _result
            OUTPUT_VARIABLE _output
            ERROR_VARIABLE  _error
            COMMAND_ECHO STDOUT
    )

    cots_git_fail("${step}" "${_result}" "${_output}" "${_error}")
endfunction()


function(cots_prepare_effekseer_source)
    if(EXISTS "${COTS_EFFEKSEER_VENDOR_DIR}/CMakeLists.txt")
        if(COTS_EFFEKSEER_SYNC_TAG AND EXISTS "${COTS_EFFEKSEER_VENDOR_DIR}/.git")
            find_package(Git REQUIRED)

            message(STATUS "Syncing Effekseer tag: ${COTS_EFFEKSEER_GIT_TAG}")

            cots_run_git(
                    "fetch Effekseer tags"
                    -C "${COTS_EFFEKSEER_VENDOR_DIR}"
                    fetch
                    --tags
                    --force
                    origin
            )

            cots_run_git(
                    "checkout Effekseer tag"
                    -C "${COTS_EFFEKSEER_VENDOR_DIR}"
                    checkout
                    --force
                    "${COTS_EFFEKSEER_GIT_TAG}"
            )
        endif()

        if(COTS_EFFEKSEER_UPDATE_SUBMODULES AND EXISTS "${COTS_EFFEKSEER_VENDOR_DIR}/.git")
            find_package(Git REQUIRED)

            message(STATUS "Updating Effekseer submodules")

            cots_run_git(
                    "update Effekseer submodules"
                    -C "${COTS_EFFEKSEER_VENDOR_DIR}"
                    submodule
                    update
                    --init
                    --recursive
            )
        endif()

        return()
    endif()

    if(NOT COTS_EFFEKSEER_AUTO_CLONE)
        message(FATAL_ERROR
                "Effekseer source not found at:\n"
                "  ${COTS_EFFEKSEER_VENDOR_DIR}\n\n"
                "Automatic cloning is disabled.\n"
                "Either enable COTS_EFFEKSEER_AUTO_CLONE or clone manually:\n"
                "  git clone --recurse-submodules --branch ${COTS_EFFEKSEER_GIT_TAG} "
                "${COTS_EFFEKSEER_REPOSITORY} vendor/effekseer\n")
    endif()

    find_package(Git REQUIRED)

    get_filename_component(_effekseer_parent_dir
            "${COTS_EFFEKSEER_VENDOR_DIR}"
            DIRECTORY)

    file(MAKE_DIRECTORY "${_effekseer_parent_dir}")

    if(EXISTS "${COTS_EFFEKSEER_VENDOR_DIR}")
        file(GLOB _effekseer_existing_files
                "${COTS_EFFEKSEER_VENDOR_DIR}/*")

        if(_effekseer_existing_files)
            message(FATAL_ERROR
                    "Effekseer vendor directory exists but is not a valid checkout:\n"
                    "  ${COTS_EFFEKSEER_VENDOR_DIR}\n\n"
                    "Delete that folder or set COTS_EFFEKSEER_VENDOR_DIR to another path\n")
        endif()

        file(REMOVE_RECURSE "${COTS_EFFEKSEER_VENDOR_DIR}")
    endif()

    message(STATUS
            "Cloning Effekseer ${COTS_EFFEKSEER_GIT_TAG} into "
            "${COTS_EFFEKSEER_VENDOR_DIR}")

    cots_run_git(
            "clone Effekseer"
            clone
            --recurse-submodules
            --branch "${COTS_EFFEKSEER_GIT_TAG}"
            "${COTS_EFFEKSEER_REPOSITORY}"
            "${COTS_EFFEKSEER_VENDOR_DIR}"
    )

    if(COTS_EFFEKSEER_UPDATE_SUBMODULES)
        cots_run_git(
                "update Effekseer submodules after clone"
                -C "${COTS_EFFEKSEER_VENDOR_DIR}"
                submodule
                update
                --init
                --recursive
        )
    endif()

    if(NOT EXISTS "${COTS_EFFEKSEER_VENDOR_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
                "Effekseer clone completed, but CMakeLists.txt was not found:\n"
                "  ${COTS_EFFEKSEER_VENDOR_DIR}/CMakeLists.txt\n")
    endif()
endfunction()


function(cots_fetch_effekseer)
    if(TARGET Effekseer AND TARGET EffekseerRendererDX12)
        return()
    endif()

    cots_prepare_effekseer_source()

    #~ disable extra targets
    set(BUILD_VIEWER        OFF CACHE BOOL "" FORCE)
    set(BUILD_TEST          OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES      OFF CACHE BOOL "" FORCE)
    set(BUILD_SAMPLES       OFF CACHE BOOL "" FORCE)

    #~ disable extra backends
    set(BUILD_DX9           OFF CACHE BOOL "" FORCE)
    set(BUILD_DX11          OFF CACHE BOOL "" FORCE)
    set(BUILD_DX12          ON  CACHE BOOL "" FORCE)
    set(BUILD_GL            OFF CACHE BOOL "" FORCE)
    set(BUILD_VULKAN        OFF CACHE BOOL "" FORCE)
    set(BUILD_METAL         OFF CACHE BOOL "" FORCE)

    #~ disable window deps
    set(BUILD_GLFW          OFF CACHE BOOL "" FORCE)
    set(BUILD_WITH_GLFW3    OFF CACHE BOOL "" FORCE)

    #~ disable sound deps
    set(USE_OPENSOUND       OFF CACHE BOOL "" FORCE)
    set(USE_XAUDIO2         OFF CACHE BOOL "" FORCE)
    set(USE_AL              OFF CACHE BOOL "" FORCE)
    set(USE_DSOUND          OFF CACHE BOOL "" FORCE)

    #~ disable plugins
    set(BUILD_UNITYPLUGIN   OFF CACHE BOOL "" FORCE)
    set(BUILD_GODOT3_PLUGIN OFF CACHE BOOL "" FORCE)

    set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)

    #~ preserve project option
    set(_cots_build_shared_libs_was_defined OFF)

    if(DEFINED BUILD_SHARED_LIBS)
        set(_cots_build_shared_libs_was_defined ON)
        set(_cots_build_shared_libs_saved "${BUILD_SHARED_LIBS}")
    endif()

    #~ prefer static libs
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    add_subdirectory(
            "${COTS_EFFEKSEER_VENDOR_DIR}"
            "${COTS_EFFEKSEER_BINARY_DIR}"
            EXCLUDE_FROM_ALL
    )

    #~ restore project option
    if(_cots_build_shared_libs_was_defined)
        set(BUILD_SHARED_LIBS
                "${_cots_build_shared_libs_saved}"
                CACHE BOOL ""
                FORCE)
    else()
        unset(BUILD_SHARED_LIBS CACHE)
        unset(BUILD_SHARED_LIBS)
    endif()

    if(NOT TARGET Effekseer)
        message(FATAL_ERROR
                "Effekseer target was not created.\n"
                "Check the selected Effekseer tag:\n"
                "  ${COTS_EFFEKSEER_GIT_TAG}\n"
                "and inspect:\n"
                "  ${COTS_EFFEKSEER_VENDOR_DIR}/CMakeLists.txt\n")
    endif()

    if(NOT TARGET EffekseerRendererDX12)
        message(FATAL_ERROR
                "EffekseerRendererDX12 target was not created.\n"
                "The DX12 backend may be disabled or the target name may have changed.\n"
                "Check BUILD_DX12 and the selected Effekseer tag:\n"
                "  ${COTS_EFFEKSEER_GIT_TAG}\n")
    endif()
endfunction()


function(cots_link_effekseer target visibility)
    if(NOT TARGET ${target})
        message(FATAL_ERROR
                "Cannot link Effekseer. Target does not exist:\n"
                "  ${target}\n")
    endif()

    cots_fetch_effekseer()

    target_link_libraries(${target} ${visibility}
            Effekseer
            EffekseerRendererDX12
    )
endfunction()
