file(GLOB_RECURSE maa_control_unit_src *.h *.hpp *.cpp ../include/MaaControlUnit/*.h)

add_library(MaaControlUnit SHARED ${maa_control_unit_src})

target_include_directories(MaaControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(MaaControlUnit MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS} ZLIB::ZLIB)
if(WIN32)
    target_link_libraries(MaaControlUnit ws2_32)
endif()

target_compile_definitions(MaaControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(MaaControlUnit MaaUtils)

install(TARGETS MaaControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    PUBLIC_HEADER DESTINATION include
    ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${maa_control_unit_src})
