set(THRIFT_COMPILER ${MAADEPS_TARGET_TOOLS}/thrift/thrift${CMAKE_EXECUTABLE_SUFFIX})

macro(GENERATE_THRIFT_LIB LIB_NAME FILENAME OUTPUTDIR SOURCES)
    file(MAKE_DIRECTORY ${OUTPUTDIR})

    execute_process(COMMAND ${THRIFT_COMPILER} --gen cpp:no_skeleton -out ${OUTPUTDIR} ${FILENAME}
                    RESULT_VARIABLE CMD_RESULT)
    if(CMD_RESULT)
        message(FATAL_ERROR "Error generating ${FILENAME} with generator ${GENERATOR}")
    endif()
    file(GLOB_RECURSE GENERATED_SOURCES ${OUTPUTDIR}/*.cpp ${OUTPUTDIR}/*.h ${OUTPUTDIR}/*.hpp)
    add_library(${LIB_NAME} STATIC ${GENERATED_SOURCES} ${FILENAME})
    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(${LIB_NAME} thrift::thrift)
    set(${LIB_NAME}_INCLUDE_DIRS
        ${OUTPUTDIR}
        PARENT_SCOPE)
    set(${SOURCES} ${GENERATED_SOURCES})

    set_target_properties(${LIB_NAME} PROPERTIES FOLDER "Generated")
    
    source_group("generated" FILES ${GENERATED_SOURCES})
    source_group("thrift" FILES ${FILENAME})
endmacro()
