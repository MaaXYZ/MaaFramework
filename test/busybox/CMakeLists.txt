add_executable(maabusybox maabusybox.cpp)

set_target_properties(maabusybox PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_compile_definitions(maabusybox PRIVATE MAA_DEBUG)
target_link_libraries(maabusybox MaaFramework HeaderOnlyLibraries)
target_include_directories(maabusybox PUBLIC ${MaaFramework_INCLUDE} "../../source")

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/controller_config.json ${CMAKE_CURRENT_BINARY_DIR}/MaaAgentBinary
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/sample/cpp/config/controller_config.json ${CMAKE_CURRENT_BINARY_DIR}/controller_config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/MaaAgentBinary ${CMAKE_CURRENT_BINARY_DIR}/MaaAgentBinary
)
add_custom_target(maabusybox_res ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/controller_config.json ${CMAKE_CURRENT_BINARY_DIR}/MaaAgentBinary)
set_property(TARGET maabusybox_res APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/controller_config.json ${CMAKE_CURRENT_BINARY_DIR}/MaaAgentBinary)
add_dependencies(maabusybox maabusybox_res)
