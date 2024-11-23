if(BUILD_NODEJS_BINDING)
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/NodeJS/node_modules)
        add_subdirectory(NodeJS)
    else()
        message("NodeJS/node_modules not found, install via `pnpm install` first.")
    endif()
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Python" DESTINATION binding)

# install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cpp" DESTINATION binding)
