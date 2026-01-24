cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

set(MAA_JS_COMMON_SOURCES
    src/utils/library.cpp
    src/apis/loader.cpp
    src/apis/global.cpp
    src/apis/job.cpp
    src/apis/resource.cpp
    src/apis/controller.cpp
    src/apis/tasker.cpp
    src/apis/context.cpp
    src/apis/callback.cpp
    src/apis/constant.cpp)

set(MAA_JS_CLIENT_SOURCES src/apis/client.cpp)
set(MAA_JS_SERVER_SOURCES src/apis/server.cpp)
set(MAA_JS_PLUGIN_SOURCES src/apis/plugin.cpp)

# quickjs

if(WITH_QUICKJS_BINDING)
    add_library(MaaQjs STATIC ${MAA_JS_COMMON_SOURCES} ${MAA_JS_CLIENT_SOURCES} ${MAA_JS_PLUGIN_SOURCES}
                              src/plugin/runtime.cpp)
    target_compile_definitions(MaaQjs PRIVATE MAA_JS_IMPL_IS_QUICKJS MAA_JS_BUILD_CLIENT MAA_JS_BUILD_PLUGIN)
    target_link_libraries(MaaQjs PRIVATE MaaInterface qjs HeaderOnlyLibraries)
    set_target_properties(MaaQjs PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(MaaQjs PROPERTIES CXX_VISIBILITY_PRESET hidden)

    add_library(MaaQjsPlugin SHARED src/plugin/entry.cpp)
    target_link_libraries(MaaQjsPlugin PRIVATE MaaFramework MaaToolkit MaaAgentClient MaaQjs HeaderOnlyLibraries)
    set_target_properties(MaaQjsPlugin PROPERTIES CXX_VISIBILITY_PRESET hidden)
endif()

# nodejs

# TODO: differ agent and client

if(WITH_NODEJS_BINDING)
    if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/node_modules)
        message(FATAL_ERROR "NodeJS/node_modules not found, install via `pnpm install` first.")
    endif()

    set(CMAKE_JS_VERSION "7.3.0")
    set(CMAKE_JS_INC
        "${CMAKE_CURRENT_LIST_DIR}/node_modules/node-api-headers/include;${CMAKE_CURRENT_LIST_DIR}/node_modules/node-addon-api"
    )

    if(WIN32)
        set(CMAKE_JS_SRC "src/utils/win_delay_load_hook.cc")
    endif()

    set(NODE_RUNTIME "node")
    set(NODE_RUNTIMEVERSION "20.11.1")

    # set(NODE_ARCH "x64")
    if(WIN32)
        set(CMAKE_JS_LIB "${CMAKE_BINARY_DIR}/node.lib")
        set(CMAKE_JS_NODELIB_DEF "${CMAKE_CURRENT_LIST_DIR}/node_modules/node-api-headers/def/node_api.def")
        set(CMAKE_JS_NODELIB_TARGET ${CMAKE_JS_LIB})
    endif()

    if(WIN32)
        set(CMAKE_SHARED_LINKER_FLAGS "/delayload:node.exe")
        link_libraries(delayimp)
    elseif(APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
    endif()

    add_definitions(-DNAPI_VERSION=8)
    add_definitions(-DNODE_API_NO_EXTERNAL_BUFFERS_ALLOWED)

    if(WIN32
       AND CMAKE_JS_NODELIB_DEF
       AND CMAKE_JS_NODELIB_TARGET)
        # Generate node.lib
        if(MSVC)
            execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET}
                                    ${CMAKE_STATIC_LINKER_FLAGS})
        else()
            execute_process(COMMAND ${CMAKE_LINKER} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET}
                                    ${CMAKE_STATIC_LINKER_FLAGS})
        endif()
    endif()

    add_library(MaaNode SHARED ${MAA_JS_COMMON_SOURCES} ${MAA_JS_CLIENT_SOURCES} ${CMAKE_JS_SRC})
    set_target_properties(MaaNode PROPERTIES PREFIX "" SUFFIX ".node")
    target_compile_definitions(MaaNode PRIVATE MAA_JS_IMPL_IS_NODEJS MAA_JS_BUILD_CLIENT)
    target_include_directories(MaaNode PRIVATE ${CMAKE_JS_INC})
    target_link_libraries(MaaNode PRIVATE MaaFramework MaaToolkit MaaAgentClient ${CMAKE_JS_LIB})
    target_compile_definitions(MaaNode PRIVATE NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS)
    set_target_properties(MaaNode PROPERTIES POSITION_INDEPENDENT_CODE ON)

    install(
        TARGETS MaaNode
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)

    if(WIN32)
        install(
            FILES $<TARGET_PDB_FILE:MaaNode>
            DESTINATION symbol
            OPTIONAL)
    endif()

    add_library(MaaNodeServer SHARED ${MAA_JS_COMMON_SOURCES} ${MAA_JS_SERVER_SOURCES} ${CMAKE_JS_SRC})
    set_target_properties(MaaNodeServer PROPERTIES PREFIX "" SUFFIX ".node")
    target_compile_definitions(MaaNodeServer PRIVATE MAA_JS_IMPL_IS_NODEJS MAA_JS_BUILD_SERVER)
    target_include_directories(MaaNodeServer PRIVATE ${CMAKE_JS_INC})
    target_link_libraries(MaaNodeServer PRIVATE MaaAgentServer MaaToolkit ${CMAKE_JS_LIB})
    target_compile_definitions(MaaNodeServer PRIVATE NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS)
    set_target_properties(MaaNodeServer PROPERTIES POSITION_INDEPENDENT_CODE ON)

    install(
        TARGETS MaaNodeServer
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib)

    if(WIN32)
        install(
            FILES $<TARGET_PDB_FILE:MaaNodeServer>
            DESTINATION symbol
            OPTIONAL)
    endif()
endif()
