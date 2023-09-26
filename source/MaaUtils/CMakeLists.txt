file(GLOB_RECURSE maa_utils_src *.h *.hpp *.cpp ../include)

add_library(MaaUtils SHARED ${maa_utils_src})
target_include_directories(MaaUtils
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(MaaUtils HeaderOnlyLibraries)
target_compile_definitions(MaaUtils PRIVATE MAA_UTILS_EXPORTS)

install(TARGETS MaaUtils
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    PUBLIC_HEADER DESTINATION include
    ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_utils_src})
