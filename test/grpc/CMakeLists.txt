file(GLOB grpc_testing_src ./*.cpp)

add_executable(GrpcTesting ${grpc_testing_src})
# 我觉得这个应该 generated 扔到 build 文件夹里去
target_include_directories(GrpcTesting PRIVATE ${CMAKE_SOURCE_DIR}/source/MaaRpc/generated/)
target_link_libraries(GrpcTesting PUBLIC MaaRpcProto protobuf::libprotobuf gRPC::grpc++)
add_dependencies(GrpcTesting MaaRpcProto)
set_target_properties(GrpcTesting PROPERTIES FOLDER Testing)

install(TARGETS GrpcTesting RUNTIME DESTINATION bin)
