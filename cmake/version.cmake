# define MAA_VERSION from git
if (NOT DEFINED MAA_VERSION)
    find_package(Git)
endif ()
if (NOT DEFINED MAA_VERSION AND GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE err
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (result EQUAL 0)
        set(MAA_VERSION "${output}")
    else ()
        message(WARNING "git rev-parse returning ${result}, output:\n${err}")
    endif ()
endif ()
if (NOT MAA_VERSION)
    set(MAA_VERSION "DEBUG_VERSION")
endif ()
message(STATUS "MAA_VERSION=${MAA_VERSION}")
add_compile_definitions(MAA_VERSION="${MAA_VERSION}")
