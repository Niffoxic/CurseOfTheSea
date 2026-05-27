function(cots_add_assets target)
    file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/assets/*")

    add_custom_target(copy_assets ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${target}>/assets
            COMMENT "Copying assets next to ${target}.exe"
    )

    add_dependencies(${target} copy_assets)
endfunction()
