cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

set(CMAKE_JS_VERSION "7.3.0")
set(CMAKE_JS_INC
    "${CMAKE_CURRENT_LIST_DIR}/node_modules/node-api-headers/include;${CMAKE_CURRENT_LIST_DIR}/node_modules/node-addon-api"
)

if(WIN32)
    set(CMAKE_JS_SRC "${CMAKE_CURRENT_LIST_DIR}/node_modules/cmake-js/lib/cpp/win_delay_load_hook.cc")
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

set(maa_common_src
    src/instance/context.cpp
    src/instance/controller.cpp
    src/instance/resource.cpp
    src/instance/tasker.cpp
    src/toolkit/config.cpp
    src/toolkit/find.cpp
    src/toolkit/pi.cpp
    src/utility/utility.cpp
    src/main.cpp

    ${CMAKE_JS_SRC}
)

# Master

add_library(MaaNode SHARED ${maa_common_src} src/agent/client.cpp)
set_target_properties(MaaNode PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(MaaNode PRIVATE ${CMAKE_JS_INC})
target_link_libraries(MaaNode ${CMAKE_JS_LIB} MaaFramework MaaToolkit MaaAgentClient)
target_compile_definitions(MaaNode PRIVATE NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS MAA_NODE_BUILD_CLIENT)
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

# Slave

add_library(MaaNodeServer SHARED ${maa_common_src} src/agent/server.cpp)
set_target_properties(MaaNodeServer PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(MaaNodeServer PRIVATE ${CMAKE_JS_INC})
target_link_libraries(MaaNodeServer ${CMAKE_JS_LIB} MaaToolkit MaaAgentServer)
target_compile_definitions(MaaNodeServer PRIVATE NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS MAA_NODE_BUILD_SERVER)
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
