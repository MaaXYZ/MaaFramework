add_custom_target(tools ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR})
set_property(
    TARGET tools
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(tools PROPERTIES FOLDER Assets)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ImageCropper
    DESTINATION tools)
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/pipeline.schema.json
    DESTINATION tools)
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/interface.schema.json
    DESTINATION tools)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/MaaDebugger
    DESTINATION tools)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/MaaDebugger/README.md)
    find_package(Python)

    if(Python_FOUND AND MSVC)
        add_custom_target(MaaDebugger ALL
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/MaaDebugger)
        add_dependencies(MaaDebugger MaaFramework MaaToolkit)

        set_target_properties(MaaDebugger PROPERTIES
            VS_DEBUGGER_COMMAND ${Python_EXECUTABLE}
            VS_DEBUGGER_COMMAND_ARGUMENTS
            "\"${CMAKE_CURRENT_SOURCE_DIR}/MaaDebugger/main.py\" \"${CMAKE_SOURCE_DIR}/source/binding/Python\" \"${CMAKE_BINARY_DIR}/bin/$<CONFIG>\"")
    endif()
endif()