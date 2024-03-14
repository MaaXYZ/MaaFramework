set(LHG_EMBEDDED
    ON
    CACHE BOOL "make lhg work in embedded mode")
add_subdirectory(LHG)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

lhggenerate(${CMAKE_CURRENT_SOURCE_DIR}/interface.json ${CMAKE_CURRENT_SOURCE_DIR}/config.json
            ${CMAKE_CURRENT_SOURCE_DIR}/info.hpp)

set(maa_http_src main.cpp extra.cpp extra.h include.h info.hpp)

add_executable(MaaHttp ${maa_http_src})
target_include_directories(MaaHttp PRIVATE ./../include)
target_link_libraries(MaaHttp LHGHeaderonly LHGSupport Boost::system MaaFramework MaaToolkit)

if(WIN32)
    target_compile_definitions(MaaHttp PRIVATE _WIN32_WINNT=0x0A00)
    target_compile_options(MaaHttp PRIVATE /bigobj)
endif()

install(
    TARGETS MaaHttp
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_http_src})
