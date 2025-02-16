file(GLOB_RECURSE maa_framework_src *.h *.hpp *.cpp ../Common/*.h ../Common/*.hpp ../Common/*.cpp)
file(GLOB_RECURSE maa_framework_header ${MAA_PUBLIC_INC}/MaaFramework/*)

add_library(MaaFramework SHARED ${maa_framework_src} ${maa_framework_header})

target_include_directories(
    MaaFramework
    PUBLIC $<BUILD_INTERFACE:${MAA_PUBLIC_INC}> $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_compile_definitions(MaaFramework PRIVATE MAA_FRAMEWORK_EXPORTS)

target_link_libraries(MaaFramework PRIVATE MaaUtils LibraryHolder ${OpenCV_LIBS} fastdeploy_ppocr
    ONNXRuntime::ONNXRuntime HeaderOnlyLibraries)

add_dependencies(MaaFramework MaaUtils LibraryHolder)

if(WITH_ADB_CONTROLLER)
    add_dependencies(MaaFramework MaaAdbControlUnit)
endif()

if(WITH_WIN32_CONTROLLER)
    add_dependencies(MaaFramework MaaWin32ControlUnit)
endif()

if(WITH_DBG_CONTROLLER)
    add_dependencies(MaaFramework MaaDbgControlUnit)
endif()

install(
    TARGETS MaaFramework
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaFramework> DESTINATION symbol OPTIONAL)
endif()

install(DIRECTORY "${MAA_PUBLIC_INC}/MaaFramework" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_framework_src})

if(WIN32)
    if(USE_MAADEPS)
        add_custom_command(
            TARGET MaaFramework
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/$<$<CONFIG:Debug>:msvc-debug/>"
            $<TARGET_FILE_DIR:MaaFramework>
            COMMAND_EXPAND_LISTS)
    else()
        add_custom_command(
            TARGET MaaFramework
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            $<TARGET_RUNTIME_DLLS:MaaFramework>
            $<TARGET_FILE_DIR:MaaFramework>
            COMMAND_EXPAND_LISTS)
    endif()
endif()
