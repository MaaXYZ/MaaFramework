if(WITH_NODEJS_BINDING OR WITH_QUICKJS_BINDING)
    add_subdirectory(NodeJS)
endif()

# install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Python" DESTINATION binding)
