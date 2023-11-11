file(GLOB_RECURSE maa_adb_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_adb_control_unit_header ../include/ControlUnit/AdbControlUnitAPI.h ../include/ControlUnit/ControlUnitAPI.h)

add_library(MaaAdbControlUnit SHARED ${maa_adb_control_unit_src} ${maa_adb_control_unit_header})

target_include_directories(MaaAdbControlUnit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include
                                                  ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(MaaAdbControlUnit MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} ZLIB::ZLIB Boost::system)
if(WIN32)
    target_link_libraries(MaaAdbControlUnit ws2_32)
endif()

target_compile_definitions(MaaAdbControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaAdbControlUnit MaaUtils)

install(
    TARGETS MaaAdbControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    #ARCHIVE DESTINATION lib
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_adb_control_unit_src})
