if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 18)
    install(
        CODE "
        execute_process(
            COMMAND \${CMAKE_SOURCE_DIR}/tools/fix_mac_rpath.sh \${CMAKE_INSTALL_PREFIX}
        )
    ")
endif()
