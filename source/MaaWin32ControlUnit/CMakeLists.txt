file(GLOB_RECURSE maa_win32_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_win32_control_unit_header ../include/ControlUnit/Win32ControlUnitAPI.h ../include/ControlUnit/ControlUnitAPI.h)

add_library(MaaWin32ControlUnit SHARED ${maa_win32_control_unit_src} ${maa_win32_control_unit_header})

target_include_directories(MaaWin32ControlUnit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include
                                                  ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(MaaWin32ControlUnit MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} ZLIB::ZLIB Boost::system)
target_link_libraries(MaaWin32ControlUnit d3d11 dxgi)

target_compile_definitions(MaaWin32ControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaWin32ControlUnit MaaUtils)

install(
    TARGETS MaaWin32ControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    #ARCHIVE DESTINATION lib
    )

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_win32_control_unit_src})
