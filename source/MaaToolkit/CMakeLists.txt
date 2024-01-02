file(GLOB_RECURSE maa_toolkit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_toolkit_header ../../include/MaaToolkit/*)

add_library(MaaToolkit SHARED ${maa_toolkit_src} ${maa_toolkit_header})

target_include_directories(
    MaaToolkit
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include> $<INSTALL_INTERFACE:include>
    PRIVATE . ../include ../../include)

target_compile_definitions(MaaToolkit PRIVATE MAA_TOOLKIT_EXPORTS)

target_link_libraries(
    MaaToolkit
    PUBLIC MaaFramework
    PRIVATE MaaUtils LibraryHolder HeaderOnlyLibraries Boost::system ${OpenCV_LIBS})

add_dependencies(MaaToolkit MaaFramework MaaUtils LibraryHolder)

install(
    TARGETS MaaToolkit
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaToolkit" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_toolkit_src})
