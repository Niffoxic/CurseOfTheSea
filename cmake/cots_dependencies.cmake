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

    # ecs and event dispatcher
    FetchContent_Declare(
            EnTT
            GIT_REPOSITORY https://github.com/skypjack/entt.git
            GIT_TAG v3.16.0
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(EnTT)
    target_link_libraries(${target} PRIVATE EnTT::EnTT)

    # JSON
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
