add_library(HeaderOnlyLibraries INTERFACE)
target_include_directories(HeaderOnlyLibraries INTERFACE include)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/MaaAgentBinary/README.md)
    add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/MaaAgentBinary
    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/MaaAgentBinary
        ${CMAKE_BINARY_DIR}/MaaAgentBinary)
    add_custom_target(AgentBinary ALL
                    DEPENDS ${CMAKE_BINARY_DIR}/MaaAgentBinary)
    set_property(
    TARGET AgentBinary
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_BINARY_DIR}/MaaAgentBinary)
    set_target_properties(AgentBinary PROPERTIES FOLDER Assets)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/MaaAgentBinary
            DESTINATION share)
else()
    message(STATUS "MaaAgentBinary not found, skipping")
endif()