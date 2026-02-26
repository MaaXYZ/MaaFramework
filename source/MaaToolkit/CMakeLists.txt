file(GLOB_RECURSE maa_toolkit_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_toolkit_header ${MAA_PUBLIC_INC}/MaaToolkit/*)

add_library(MaaToolkit SHARED ${maa_toolkit_src} ${maa_toolkit_header})

target_include_directories(
    MaaToolkit
    PUBLIC $<BUILD_INTERFACE:${MAA_PUBLIC_INC}> $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_compile_definitions(MaaToolkit PRIVATE MAA_TOOLKIT_EXPORTS)

target_link_libraries(MaaToolkit PRIVATE MaaUtils LibraryHolder Boost::system ${OpenCV_LIBS})

if(LINUX)
    target_link_libraries(MaaToolkit PRIVATE pthread dl)
endif()

if(APPLE)
    target_link_options(MaaToolkit PRIVATE "-Wl,-U,_MaaGlobalSetOption")
elseif(WIN32)
    set(FWLIB_DEF ${CMAKE_CURRENT_LIST_DIR}/DelayLoad/MaaFrameworkProvider.def)
    set(FWLIB_TARGET ${CMAKE_BINARY_DIR}/MaaFrameworkProvider.lib)
    if(MSVC)
        execute_process(COMMAND ${CMAKE_AR} /def:${FWLIB_DEF} /out:${FWLIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
    else()
        execute_process(COMMAND ${CMAKE_LINKER} /def:${FWLIB_DEF} /out:${FWLIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
    endif()

    target_link_options(MaaToolkit PRIVATE "/delayload:MaaFrameworkProvider.dll")
    target_link_libraries(MaaToolkit PRIVATE delayimp ${FWLIB_TARGET})
    target_sources(MaaToolkit PRIVATE DelayLoad/Hook.cpp)
endif()

add_dependencies(MaaToolkit MaaUtils LibraryHolder)

install(
    TARGETS MaaToolkit
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)

if(WIN32)
    install(
        FILES $<TARGET_PDB_FILE:MaaToolkit>
        DESTINATION symbol
        OPTIONAL)
endif()

install(DIRECTORY "${MAA_PUBLIC_INC}/MaaToolkit" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_toolkit_src})
