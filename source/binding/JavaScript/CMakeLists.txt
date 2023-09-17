file(GLOB_RECURSE maa_js_src *.h *.hpp *.cpp)

add_library(MaaJSExtra SHARED ${maa_js_src})
target_link_libraries(MaaJSExtra MaaFramework)
target_compile_definitions(MaaJSExtra PRIVATE MAA_BINDING_EXTRA_EXPORTS)

install(TARGETS MaaJSExtra
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    PUBLIC_HEADER DESTINATION include
    ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${maa_js_src})
