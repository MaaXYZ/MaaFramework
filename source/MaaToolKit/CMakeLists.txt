file(GLOB_RECURSE maa_toolkit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_toolkit_header ../../include/MaaToolKit/*)

add_library(MaaToolKit SHARED ${maa_toolkit_src} ${maa_toolkit_header})

target_include_directories(
    MaaToolKit
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include> $<INSTALL_INTERFACE:include>
    PRIVATE . ../include ../../include)

target_compile_definitions(MaaToolKit PRIVATE MAA_TOOLKIT_EXPORTS)

target_link_libraries(
    MaaToolKit
    PUBLIC MaaFramework
    PRIVATE MaaUtils LibraryHolder HeaderOnlyLibraries Boost::system)

add_dependencies(MaaToolKit MaaFramework MaaUtils LibraryHolder)

install(
    TARGETS MaaToolKit
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaToolKit" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_toolkit_src})
