file(GLOB_RECURSE maa_toolkit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_toolkit_header ../../include/MaaToolKit/*)


add_library(MaaToolKit SHARED ${maa_toolkit_src} ${maa_toolkit_header})

set(MaaToolKit_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaToolKit)

target_include_directories(
    MaaToolKit
    INTERFACE ../../include
    PRIVATE . ../include ../../include)

target_compile_definitions(MaaToolKit PRIVATE MAA_TOOLKIT_EXPORTS)

target_link_libraries(MaaToolKit MaaFramework MaaUtils HeaderOnlyLibraries Boost::system)

add_dependencies(MaaToolKit MaaFramework MaaUtils)

install(
    TARGETS MaaToolKit
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaToolKit" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_toolkit_src})
