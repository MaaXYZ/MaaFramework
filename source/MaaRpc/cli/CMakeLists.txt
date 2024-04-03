file(GLOB_RECURSE maa_rpc_cli_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_rpc_cli_header ${MAA_PUBLIC_INC}/MaaRpc/*)

add_executable(MaaRpcCli ${maa_rpc_cli_src} ${maa_rpc_cli_header})
target_include_directories(MaaRpcCli
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PUBLIC_INC})
target_link_libraries(MaaRpcCli MaaRpc)
add_dependencies(MaaRpcCli MaaRpc)

install(TARGETS MaaRpcCli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)
