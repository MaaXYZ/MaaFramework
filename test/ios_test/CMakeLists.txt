add_executable(MaaIOSInputMappingTest ios_input_mapping_test.cpp)
target_include_directories(MaaIOSInputMappingTest PRIVATE "${CMAKE_SOURCE_DIR}/source/MaaIOSControlUnit")
add_test(NAME MaaIOSInputMappingTest COMMAND MaaIOSInputMappingTest)
set_target_properties(MaaIOSInputMappingTest PROPERTIES FOLDER Testing)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME MaaIOSXdbIntegrationTest COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/xdb_integration_test.py ${CMAKE_SOURCE_DIR}
    $<TARGET_FILE:MaaFramework> $<TARGET_FILE:MaaToolkit>)
set_tests_properties(MaaIOSXdbIntegrationTest PROPERTIES TIMEOUT 30)
