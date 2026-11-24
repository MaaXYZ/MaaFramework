add_executable(quickjs_runtime_test main.cpp)
target_compile_features(quickjs_runtime_test PRIVATE cxx_std_20)
target_include_directories(quickjs_runtime_test PRIVATE "${CMAKE_SOURCE_DIR}/source/binding/NodeJS/src/plugin" ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})
target_link_libraries(quickjs_runtime_test PRIVATE MaaQjs MaaFramework MaaAgentClient)
set_target_properties(quickjs_runtime_test PROPERTIES FOLDER Testing)

add_test(NAME quickjs_client_ctor COMMAND quickjs_runtime_test "${CMAKE_CURRENT_SOURCE_DIR}/client_ctor_failure.js")
set_tests_properties(quickjs_client_ctor PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
