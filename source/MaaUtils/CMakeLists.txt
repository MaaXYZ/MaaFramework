file(GLOB_RECURSE maa_utils_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_utils_header ${MAA_PRIVATE_INC}/Utils/*)

add_library(MaaUtils SHARED ${maa_utils_src} ${maa_utils_header})
target_include_directories(MaaUtils
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})
target_link_libraries(MaaUtils PRIVATE HeaderOnlyLibraries Boost::system ${OpenCV_LIBS})
target_compile_definitions(MaaUtils PRIVATE MAA_UTILS_EXPORTS)

install(
    TARGETS MaaUtils
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin # ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_utils_src})
