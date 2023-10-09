file(GLOB maa_rpc_test_src ./*.cpp)

add_executable(MaaRpcTest ${maa_proto_src} ${maa_rpc_test_src})
target_include_directories(MaaRpcTest PRIVATE ../../../include)
target_link_libraries(MaaRpcTest
  PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${maa_rpc_test_src})
