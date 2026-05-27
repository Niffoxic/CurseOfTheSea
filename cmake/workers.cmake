include(FetchContent)

function(cots_fetch_concurrentqueue)
    if(TARGET concurrentqueue)
        return()
    endif()

    FetchContent_Declare(
            concurrentqueue
            GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
            GIT_TAG v1.0.4
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(concurrentqueue)
endfunction()
