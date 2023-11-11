file(GLOB_RECURSE maa_utils_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_utils_header ../include/Utils/*)

add_library(MaaUtils SHARED ${maa_utils_src} ${maa_utils_header})
target_include_directories(MaaUtils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include
                                            ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(MaaUtils HeaderOnlyLibraries Boost::system)
target_compile_definitions(MaaUtils PRIVATE MAA_UTILS_EXPORTS)

install(
    TARGETS MaaUtils
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    #ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_utils_src})
