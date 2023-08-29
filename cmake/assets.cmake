set(maa_assets_target ${CMAKE_BINARY_DIR})

add_custom_command(OUTPUT ${maa_assets_target}/MaaAgentBinary
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets/config/MaaAgentBinary ${maa_assets_target}/MaaAgentBinary
)
add_custom_target(AssetsAgentBinary ALL DEPENDS ${maa_assets_target}/MaaAgentBinary)
set_property(TARGET AssetsAgentBinary APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${maa_assets_target}/MaaAgentBinary)
set_target_properties(AssetsAgentBinary PROPERTIES FOLDER Assets)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/config/MaaAgentBinary DESTINATION share)

add_custom_command(OUTPUT ${maa_assets_target}/resource
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets/resource ${maa_assets_target}/resource
)
add_custom_target(AssetsResource ALL DEPENDS ${maa_assets_target}/resource)
set_property(TARGET AssetsResource APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${maa_assets_target}/resource)
set_target_properties(AssetsResource PROPERTIES FOLDER Assets)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/resource DESTINATION share)
