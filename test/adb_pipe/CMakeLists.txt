add_executable(AdbPipeTesting main.cpp)
target_include_directories(AdbPipeTesting PRIVATE
    ${PROJECT_SOURCE_DIR}/source/MaaAdbControlUnit
    ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})
target_link_libraries(AdbPipeTesting PRIVATE MaaAdbControlUnit MaaUtils)
set_target_properties(AdbPipeTesting PROPERTIES FOLDER Testing)

foreach(case complete empty grace_exit timeout_retry nonzero eof_before_exit inherited_writer)
    add_test(NAME adb_pipe_${case} COMMAND AdbPipeTesting ${case})
    set_tests_properties(adb_pipe_${case} PROPERTIES TIMEOUT 8)
endforeach()
