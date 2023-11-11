file(GLOB_RECURSE maa_dbg_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_dbg_control_unit_header ../include/ControlUnit/DbgControlUnitAPI.h ../include/ControlUnit/ControlUnitAPI.h)

add_library(MaaDbgControlUnit SHARED ${maa_dbg_control_unit_src} ${maa_dbg_control_unit_header})

target_include_directories(MaaDbgControlUnit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include
                                                  ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(MaaDbgControlUnit MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS})

target_compile_definitions(MaaDbgControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaDbgControlUnit MaaUtils)

install(
    TARGETS MaaDbgControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    #ARCHIVE DESTINATION lib
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_dbg_control_unit_src})
