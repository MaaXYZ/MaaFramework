set(maa_assets_target ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_custom_command(OUTPUT ${maa_assets_target}/controller_config.json
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/assets/config/controller_config.json ${maa_assets_target}/controller_config.json
)
add_custom_target(AssetsControllerConfig ALL DEPENDS ${maa_assets_target}/controller_config.json)
set_property(TARGET AssetsControllerConfig APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${maa_assets_target}/controller_config.json)

add_custom_command(OUTPUT ${maa_assets_target}/MaaAgentBinary
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/MaaAgentBinary ${maa_assets_target}/MaaAgentBinary
)
add_custom_target(AssetsAgentBinary ALL DEPENDS ${maa_assets_target}/MaaAgentBinary)
set_property(TARGET AssetsAgentBinary APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${maa_assets_target}/MaaAgentBinary)
