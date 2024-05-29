add_library(HeaderOnlyLibraries INTERFACE)
target_include_directories(HeaderOnlyLibraries INTERFACE include)
add_library(EmulatorExtras INTERFACE)
target_include_directories(EmulatorExtras INTERFACE EmulatorExtras)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/MaaAgentBinary/README.md)
    add_custom_target(AgentBinary ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/MaaAgentBinary)
    # set_property( TARGET AgentBinary APPEND PROPERTY ADDITIONAL_CLEAN_FILES
    # ${CMAKE_CURRENT_SOURCE_DIR}/MaaAgentBinary)
    set_target_properties(AgentBinary PROPERTIES FOLDER Assets)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/MaaAgentBinary DESTINATION share)
else()
    message(STATUS "MaaAgentBinary not found, skipping")
endif()
