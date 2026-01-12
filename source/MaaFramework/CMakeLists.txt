file(GLOB_RECURSE maa_framework_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_common_src ../Common/*.h ../Common/*.hpp ../Common/*.cpp)
file(GLOB_RECURSE maa_framework_header ${MAA_PUBLIC_INC}/MaaFramework/*)

add_library(MaaFramework SHARED ${maa_common_src} ${maa_framework_src} ${maa_framework_header})

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

if(WITH_GAMEPAD_CONTROLLER)
    add_dependencies(MaaFramework MaaGamepadControlUnit)
endif()

if(WITH_PLAYCOVER_CONTROLLER)
    add_dependencies(MaaFramework MaaPlayCoverControlUnit)
endif()

if(WITH_DBG_CONTROLLER)
    add_dependencies(MaaFramework MaaDbgControlUnit)
endif()

if(WITH_CUSTOM_CONTROLLER)
    add_dependencies(LibraryHolder MaaCustomControlUnit)
endif()

# 提供一个仅包含include而不链接的target, 方便处理client/server通用代码
add_library(MaaInterface INTERFACE)
target_include_directories(MaaInterface INTERFACE $<BUILD_INTERFACE:${MAA_PUBLIC_INC}> $<INSTALL_INTERFACE:include>)

install(
    TARGETS MaaFramework MaaInterface
    EXPORT MaaFrameworkTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:MaaFramework> DESTINATION symbol OPTIONAL)
endif()

install(DIRECTORY "${MAA_PUBLIC_INC}/MaaFramework" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_framework_src})
source_group(PREFIX "Common" TREE ${CMAKE_CURRENT_SOURCE_DIR}/../Common FILES ${maa_common_src})

if(WIN32)
    add_custom_command(
        TARGET MaaFramework
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/$<$<CONFIG:Debug>:msvc-debug/>"
        $<TARGET_FILE_DIR:MaaFramework>
        COMMAND_EXPAND_LISTS)
endif()
