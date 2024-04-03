file(GLOB grpc_testing_src ./*.cpp)

add_executable(GrpcTesting ${grpc_testing_src})
target_include_directories(GrpcTesting PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../../source/MaaRpc/generated/)
target_link_libraries(GrpcTesting PUBLIC RpcProto protobuf::libprotobuf gRPC::grpc++)
add_dependencies(GrpcTesting RpcProto)
set_target_properties(GrpcTesting PROPERTIES FOLDER Testing)

install(TARGETS GrpcTesting RUNTIME DESTINATION bin)
