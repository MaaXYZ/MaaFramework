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

if(WITH_QUICKJS_BINDING)
    add_subdirectory(quickjs EXCLUDE_FROM_ALL)
    add_library(qjs_headers INTERFACE)
    target_include_directories(qjs_headers INTERFACE quickjs)

    if(MSVC)
        target_compile_options(
            qjs
            PRIVATE /wd4146
            /wd4668
            /wd4132
            /wd4334
            /wd4702
            /wd4701
            /wd4746
            /wd4244)
    elseif(LINUX)
        target_compile_options(qjs PRIVATE -Wno-error=unused-command-line-argument)
    endif()
endif()

add_custom_target(MaaPlugin ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/MaaPlugin)
set_target_properties(MaaPlugin PROPERTIES FOLDER Assets)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/MaaPlugin DESTINATION share)
