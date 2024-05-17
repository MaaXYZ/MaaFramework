generate_thrift_lib(
    ThriftControlUnitThrift
    ${MAA_PUBLIC_INC}/MaaFramework/Instance/ThriftController.thrift
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    maa_thrift_controlunit_thrift_src)

file(GLOB_RECURSE maa_thrift_control_unit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_thrift_control_unit_header ${MAA_PRIVATE_INC}/ControlUnit/ThriftControlUnitAPI.h ${MAA_PRIVATE_INC}/ControlUnit/ControlUnitAPI.h)

add_library(MaaThriftControlUnit SHARED ${maa_thrift_control_unit_src} ${maa_thrift_control_unit_header})

target_include_directories(MaaThriftControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaThriftControlUnit MaaUtils ThriftControlUnitThrift HeaderOnlyLibraries ${OpenCV_LIBS})

target_compile_definitions(MaaThriftControlUnit PRIVATE MAA_CONTROL_UNIT_EXPORTS)

add_dependencies(ThriftControlUnitThrift MaaUtils)

install(
    TARGETS MaaThriftControlUnit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin

    # ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_thrift_control_unit_src})
