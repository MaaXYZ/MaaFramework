include(scripts/gen.cmake)

file(GLOB maa_proto ../../include/Interface/proto/*.proto)
file(GLOB maa_proto_src generated/*.h generated/*.cc)
file(GLOB maa_rpc_src implement/*.h implement/*.cpp API/*.cpp)

add_library(MaaRpc SHARED ${maa_proto_src} ${maa_rpc_src})
target_compile_definitions(MaaRpc PRIVATE MAA_RPC_EXPORTS)
target_include_directories(MaaRpc INTERFACE ../../include PRIVATE .  ../include ../../include ${CMAKE_CURRENT_SOURCE_DIR}/generated)
target_link_libraries(MaaRpc
    PUBLIC
        MaaToolKit
        protobuf::libprotobuf
        gRPC::grpc++
)

install(TARGETS MaaRpc
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaRpc" DESTINATION "include")

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${maa_proto_src} ${maa_rpc_src})

add_subdirectory(cli)
add_subdirectory(test)
