file(GLOB_RECURSE maa_mac_control_unit_src *.h *.hpp *.cpp *.mm)
file(GLOB_RECURSE maa_mac_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/MacControlUnitAPI.h
     ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

find_library(COCOA_LIBRARY Cocoa)

add_library(MaaMacControlUnit SHARED ${maa_mac_control_unit_src} ${maa_mac_control_unit_header})

target_include_directories(MaaMacControlUnit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaMacControlUnit MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} ZLIB::ZLIB Boost::system)
target_link_libraries(MaaMacControlUnit ${COCOA_LIBRARY})

target_compile_definitions(MaaMacControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaMacControlUnit MaaUtils)

install(
    TARGETS MaaMacControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin # ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_mac_control_unit_src})
