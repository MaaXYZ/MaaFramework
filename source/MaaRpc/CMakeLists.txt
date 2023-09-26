file(GLOB maa_proto ../../include/MaaFramework/interfaces/*.proto)
file(GLOB maa_proto_src generated/*.h generated/*.cc)
file(GLOB maa_rpc_src implement/*.h implement/*.cpp)

add_executable(MaaRPC ${maa_proto_src} ${maa_rpc_src})
target_include_directories(MaaRPC PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/generated)
target_link_libraries(MaaRPC
    PUBLIC
        MaaToolKit
        protobuf::libprotobuf-lite
        gRPC::grpc++
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${maa_proto_src} ${maa_rpc_src})
