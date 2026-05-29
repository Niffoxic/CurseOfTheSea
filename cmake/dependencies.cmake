include(FetchContent)

function(cots_add_general_dependencies target)
    # logger
    FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG 79524dd
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(spdlog)
    target_link_libraries(${target} PRIVATE spdlog::spdlog)

    # ecs system
    FetchContent_Declare(
            EnTT
            GIT_REPOSITORY https://github.com/skypjack/entt.git
            GIT_TAG v3.16.0
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(EnTT)
    target_link_libraries(${target} PUBLIC EnTT::EnTT)

    # json
    FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.3
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
    target_link_libraries(${target} PRIVATE nlohmann_json::nlohmann_json)

    # D3D12 Memory Allocator
    FetchContent_Declare(
            d3d12ma
            GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git
            GIT_TAG v3.1.0
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(d3d12ma)
    target_link_libraries(${target} PRIVATE D3D12MemoryAllocator)

    # stb single header png decode
    FetchContent_Declare(
            stb
            GIT_REPOSITORY https://github.com/nothings/stb.git
            GIT_TAG f0569113c93ad095470c54bf34a17b36646bbbb5
            GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(stb)
    target_include_directories(${target} PRIVATE ${stb_SOURCE_DIR})

    # DirectXTex
    set(BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
    set(BUILD_SAMPLE   OFF CACHE BOOL "" FORCE)
    set(BUILD_DX11     OFF CACHE BOOL "" FORCE)
    set(BUILD_DX12     ON  CACHE BOOL "" FORCE)
    FetchContent_Declare(
            DirectXTex
            GIT_REPOSITORY https://github.com/microsoft/DirectXTex.git
            GIT_TAG sep2024
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(DirectXTex)

    if(TARGET Microsoft::DirectXTex)
        target_link_libraries(${target} PRIVATE Microsoft::DirectXTex)
    elseif(TARGET DirectXTex)
        target_link_libraries(${target} PRIVATE DirectXTex)
    else()
        message(FATAL_ERROR "DirectXTex was fetched but no known DirectXTex CMake target was found!")
    endif()

    # fastgltf glTF parser
    set(FASTGLTF_COMPILE_AS_CPP20 ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
            fastgltf
            GIT_REPOSITORY https://github.com/spnda/fastgltf.git
            GIT_TAG v0.8.0
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(fastgltf)
    target_link_libraries(${target} PRIVATE fastgltf::fastgltf)

    # meshoptimizer
    FetchContent_Declare(
            meshoptimizer
            GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
            GIT_TAG v0.21
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(meshoptimizer)
    target_link_libraries(${target} PRIVATE meshoptimizer)

    include(animation)

    cots_link_ozz (${target} PUBLIC)
    cots_link_ufbx(${target} PRIVATE)

    # Tracy
    if(COTS_USE_TRACY)
        FetchContent_Declare(
                tracy
                GIT_REPOSITORY https://github.com/wolfpld/tracy.git
                GIT_TAG v0.11.1
                GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(tracy)
        target_link_libraries(${target} PRIVATE Tracy::TracyClient)
    endif()
endfunction()
