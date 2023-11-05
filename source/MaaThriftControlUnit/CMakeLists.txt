generate_thrift_lib(MaaThriftControlUnit ${PROJECT_SOURCE_DIR}/include/Interface/ThriftControlUnit.thrift
                    ${CMAKE_CURRENT_BINARY_DIR}/MaaThriftControlUnit maa_thrift_controlunit_src)

source_group(TREE ${CMAKE_CURRENT_BINARY_DIR} FILES ${maa_thrift_controlunit_src})
