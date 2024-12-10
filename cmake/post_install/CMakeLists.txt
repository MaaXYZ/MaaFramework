if(APPLE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 18)
        install(
            CODE "
            execute_process(
                COMMAND \${CMAKE_SOURCE_DIR}/tools/mac_post_install.sh \${CMAKE_INSTALL_PREFIX} -r
            )
        ")
    endif()

    install(
        CODE "
        execute_process(
            COMMAND \${CMAKE_SOURCE_DIR}/tools/mac_post_install.sh \${CMAKE_INSTALL_PREFIX} -d
        )
    " CONFIGURATIONS RelWithDebInfo)
endif()
