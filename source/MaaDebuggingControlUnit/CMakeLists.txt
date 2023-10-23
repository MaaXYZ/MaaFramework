file(GLOB_RECURSE maa_debugging_control_unit_src *.h *.hpp *.cpp ../include)

add_library(MaaDebuggingControlUnit SHARED ${maa_debugging_control_unit_src})

target_include_directories(MaaDebuggingControlUnit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include
                                                  ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(MaaDebuggingControlUnit MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS})

target_compile_definitions(MaaDebuggingControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaDebuggingControlUnit MaaUtils)

install(
    TARGETS MaaDebuggingControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    #ARCHIVE DESTINATION lib
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_debugging_control_unit_src})
