include(FetchContent)

function(cots_add_d3d12 target)
    # dx12 agility sdk
    set(AGILITY_SDK_VERSION "1.616.0")
    set(AGILITY_SDK_BUILD_VERSION 616)

    FetchContent_Declare(
            agility_sdk
            URL "https://globalcdn.nuget.org/packages/microsoft.direct3d.d3d12.${AGILITY_SDK_VERSION}.nupkg"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(agility_sdk)

    target_include_directories(${target} PRIVATE
            ${agility_sdk_SOURCE_DIR}/build/native/include
    )

    target_compile_definitions(${target} PRIVATE
            COTS_AGILITY_SDK_VERSION=${AGILITY_SDK_BUILD_VERSION}
    )
    target_link_libraries(${target} PRIVATE d3d12 dxgi dxguid winmm)

    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${target}>/D3D12

            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${agility_sdk_SOURCE_DIR}/build/native/bin/x64/D3D12Core.dll
            ${agility_sdk_SOURCE_DIR}/build/native/bin/x64/d3d12SDKLayers.dll
            $<TARGET_FILE_DIR:${target}>/D3D12

            COMMENT "Copying Agility SDK DLLs to D3D12/"
    )

    # DirectX Shader Compiler
    set(DXC_VERSION "1.9.2602.17")
    FetchContent_Declare(
            dxc
            URL "https://globalcdn.nuget.org/packages/microsoft.direct3d.dxc.${DXC_VERSION}.nupkg"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(dxc)

    target_include_directories(${target} PRIVATE
            ${dxc_SOURCE_DIR}/build/native/include
    )
    target_link_libraries(${target} PRIVATE
            ${dxc_SOURCE_DIR}/build/native/lib/x64/dxcompiler.lib
    )
    # dxcompiler.dll compiles and dxil.dll signs
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${dxc_SOURCE_DIR}/build/native/bin/x64/dxcompiler.dll
            ${dxc_SOURCE_DIR}/build/native/bin/x64/dxil.dll
            $<TARGET_FILE_DIR:${target}>
            COMMENT "Copying DXC runtime DLLs"
    )
endfunction()
