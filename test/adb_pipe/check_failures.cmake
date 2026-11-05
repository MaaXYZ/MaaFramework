# A timed-out child is terminated during cleanup: both failure facts must survive.
execute_process(COMMAND "${TEST_EXECUTABLE}" timeout_retry
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 5)
if(NOT "${result}" STREQUAL "0"
    OR NOT output MATCHES "child stdout incomplete"
    OR NOT output MATCHES "child return error")
    message(FATAL_ERROR "Missing failure diagnostics (${result}):\n${output}\n${error}")
endif()

# Exercise the fork-error branch without depending on host process exhaustion.
# Require the specific setup failure, rather than accepting any nonzero exit.
execute_process(COMMAND "${TEST_EXECUTABLE}" inherited_writer_fork_failure
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 5)
if(NOT "${result}" STREQUAL "1"
    OR NOT error MATCHES "stdout-holding descendant did not confirm readiness")
    message(FATAL_ERROR "Fork failure was not rejected (${result}):\n${output}\n${error}")
endif()
