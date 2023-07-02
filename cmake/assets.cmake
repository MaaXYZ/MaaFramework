add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/controller_config.json
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/assets/config/controller_config.json ${CMAKE_BINARY_DIR}/controller_config.json
)
add_custom_target(AssetsControllerConfig ALL DEPENDS ${CMAKE_BINARY_DIR}/controller_config.json)
set_property(TARGET AssetsControllerConfig APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_BINARY_DIR}/controller_config.json)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/MaaAgentBinary
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/MaaAgentBinary ${CMAKE_BINARY_DIR}/MaaAgentBinary
)
add_custom_target(AssetsAgentBinary ALL DEPENDS ${CMAKE_BINARY_DIR}/MaaAgentBinary)
set_property(TARGET AssetsAgentBinary APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_BINARY_DIR}/MaaAgentBinary)
