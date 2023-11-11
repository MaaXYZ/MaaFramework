set(PROTO_COMPILER ${MAADEPS_HOST_TOOLS}/protobuf/protoc${CMAKE_EXECUTABLE_SUFFIX})
set(PROTO_GRPC_PLUGIN ${MAADEPS_HOST_TOOLS}/grpc/grpc_cpp_plugin${CMAKE_EXECUTABLE_SUFFIX})

macro(GENERATE_GRPC_LIB LIB_NAME IN_DIR OUT_DIR GEN_SRCS)
    file(MAKE_DIRECTORY ${OUT_DIR})
    file(GLOB_RECURSE PROTOS ${IN_DIR}/*.proto)

    foreach(PROTO ${PROTOS})
        string(REGEX REPLACE "^${IN_DIR}" ${OUT_DIR} PROTO_TIMESTAMP ${PROTO})
        if(${PROTO} IS_NEWER_THAN ${PROTO_TIMESTAMP}.timestamp)
            message("generate ${PROTO}")
            execute_process(COMMAND ${PROTO_COMPILER} "--proto_path=${IN_DIR}" "--cpp_out=${OUT_DIR}" ${PROTO})
            execute_process(COMMAND ${PROTO_COMPILER} "--proto_path=${IN_DIR}" "--grpc_out=${OUT_DIR}"
                                    "--plugin=protoc-gen-grpc=${PROTO_GRPC_PLUGIN}" ${PROTO})
            file(TOUCH ${PROTO_TIMESTAMP}.timestamp)
        endif()
    endforeach()

    file(GLOB_RECURSE SRCS ${OUT_DIR}/*.pb.*)
    add_library(${LIB_NAME} STATIC ${SRCS} ${PROTOS})
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(${LIB_NAME} protobuf::libprotobuf gRPC::grpc++)
    set(${LIB_NAME}_INCLUDE_DIRS ${OUT_DIR})
    set(${GEN_SRCS} ${SRCS})
    
    set_target_properties(${LIB_NAME} PROPERTIES FOLDER "Generated")
    
    source_group("generated" FILES ${SRCS})
    source_group("proto" FILES ${PROTOS})
endmacro()
