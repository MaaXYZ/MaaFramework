file(GLOB_RECURSE maa_ios_control_unit_src *.h *.hpp *.cpp)
list(FILTER maa_ios_control_unit_src EXCLUDE REGEX "/Protocol/")
file(GLOB_RECURSE maa_ios_control_unit_header ${MAA_PUBLIC_INC}/MaaControlUnit/IOSControlUnitAPI.h ${MAA_PUBLIC_INC}/MaaControlUnit/ControlUnitAPI.h)

add_library(MaaIOSControlUnit SHARED ${maa_ios_control_unit_src} ${maa_ios_control_unit_header})

target_include_directories(MaaIOSControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaIOSControlUnit PRIVATE MaaUtils MaaIOSProtocol ${OpenCV_LIBS})

target_compile_definitions(MaaIOSControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaIOSControlUnit MaaUtils)

install(
    TARGETS MaaIOSControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin

    # ARCHIVE DESTINATION lib
)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaIOSControlUnit> DESTINATION symbol OPTIONAL)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_ios_control_unit_src})
