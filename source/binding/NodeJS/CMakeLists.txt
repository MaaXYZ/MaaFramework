cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

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

include_directories(${CMAKE_JS_INC})

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

set(maa_node_cppm
    module/MaaFramework.cppm
    module/MaaToolkit.cppm

    src/napi.cppm
    src/stdmock.cppm

    src/agent/agent.cppm
    src/include/cb.cppm
    src/include/info.cppm
    src/include/utils.cppm
    src/include/wrapper.cppm
    src/instance/context.cppm
    src/instance/controller.cppm
    src/instance/resource.cppm
    src/instance/tasker.cppm
    src/toolkit/config.cppm
    src/toolkit/find.cppm
    src/toolkit/pi.cppm
    src/utility/utility.cppm
)

add_library(MaaNodeCommon STATIC ${CMAKE_JS_SRC} src/main.cpp)
target_sources(MaaNodeCommon PUBLIC FILE_SET cppms TYPE CXX_MODULES FILES ${maa_node_cppm})
target_include_directories(MaaNodeCommon PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../include)
target_compile_definitions(MaaNodeCommon PRIVATE NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS)
set_target_properties(MaaNodeCommon PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Master

add_library(MaaNode SHARED src/agent/client.cpp src/reg.cpp)
target_sources(MaaNode PUBLIC FILE_SET cppms TYPE CXX_MODULES FILES module/MaaAgentClient.cppm)
set_target_properties(MaaNode PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(MaaNode MaaNodeCommon ${CMAKE_JS_LIB} MaaFramework MaaToolkit MaaAgentClient)
target_compile_definitions(MaaNode PRIVATE NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS)

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

add_library(MaaNodeServer SHARED src/agent/server.cpp src/reg.cpp)
target_sources(MaaNodeServer PUBLIC FILE_SET cppms TYPE CXX_MODULES FILES module/MaaAgentServer.cppm)
set_target_properties(MaaNodeServer PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(MaaNodeServer MaaNodeCommon ${CMAKE_JS_LIB} MaaToolkit MaaAgentServer)
target_compile_definitions(MaaNodeServer PRIVATE NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS)

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
