file(GLOB maa_rpc_cli_src ./*.cpp)

add_executable(MaaRpcCli ${maa_rpc_cli_src})
target_include_directories(MaaRpcCli PRIVATE ../../../include)
target_link_libraries(MaaRpcCli MaaRpc)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${maa_rpc_cli_src})
