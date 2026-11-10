add_executable(AdbPipeTesting main.cpp)
add_executable(AdbNetcatTesting netcat.cpp)
find_package(Threads REQUIRED)
foreach(test_target AdbPipeTesting AdbNetcatTesting)
    target_include_directories(${test_target} PRIVATE
        ${PROJECT_SOURCE_DIR}/source/MaaAdbControlUnit
        ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})
    target_link_libraries(${test_target} PRIVATE MaaAdbControlUnit MaaUtils Threads::Threads)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # The release sysroot predates glibc's merge of librt into libc.
        target_link_libraries(${test_target} PRIVATE rt)
    endif()
    set_target_properties(${test_target} PROPERTIES FOLDER Testing)
endforeach()

foreach(case complete empty grace_exit timeout_retry nonzero eof_before_exit inherited_writer)
    add_test(NAME adb_pipe_${case} COMMAND AdbPipeTesting ${case})
    set_tests_properties(adb_pipe_${case} PROPERTIES TIMEOUT 8)
endforeach()

add_test(NAME adb_pipe_failure_checks
    COMMAND ${CMAKE_COMMAND} -DTEST_EXECUTABLE=$<TARGET_FILE:AdbPipeTesting>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_failures.cmake)
set_tests_properties(adb_pipe_failure_checks PROPERTIES TIMEOUT 8)

foreach(case bind_failure invalid_address loopback fallback)
    add_test(NAME adb_pipe_netcat_${case} COMMAND AdbNetcatTesting ${case})
    set_tests_properties(adb_pipe_netcat_${case} PROPERTIES TIMEOUT 8)
endforeach()
