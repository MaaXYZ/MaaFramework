file(GLOB_RECURSE maa_toolkit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_toolkit_header ${MAA_PUBLIC_INC}/MaaToolkit/*)

add_library(MaaToolkit SHARED ${maa_toolkit_src} ${maa_toolkit_header})

target_include_directories(
    MaaToolkit
    PUBLIC $<BUILD_INTERFACE:${MAA_PUBLIC_INC}> $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_compile_definitions(MaaToolkit PRIVATE MAA_TOOLKIT_EXPORTS)

target_link_libraries(
    MaaToolkit PRIVATE MaaUtils MaaFramework LibraryHolder Boost::system ${OpenCV_LIBS})

if(LINUX)
    find_library(Wayland_LIBS wayland-client)
    find_library(FFI_LIBS ffi)
    target_link_libraries(MaaToolkit PRIVATE pthread dl ${Wayland_LIBS} ${FFI_LIBS})
endif()

add_dependencies(MaaToolkit MaaUtils MaaFramework LibraryHolder)

install(
    TARGETS MaaToolkit
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaToolkit> DESTINATION symbol OPTIONAL)
endif()

install(DIRECTORY "${MAA_PUBLIC_INC}/MaaToolkit" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_toolkit_src})
