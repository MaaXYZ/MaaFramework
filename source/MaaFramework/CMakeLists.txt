file(GLOB_RECURSE maa_framework_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_framework_header ${MAA_PUBLIC_INC}/MaaFramework/*)

add_library(MaaFramework SHARED ${maa_framework_src} ${maa_framework_header})

target_include_directories(
    MaaFramework
    PUBLIC $<BUILD_INTERFACE:${MAA_PUBLIC_INC}> $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_compile_definitions(MaaFramework PRIVATE MAA_FRAMEWORK_EXPORTS)

target_link_libraries(MaaFramework PRIVATE MaaUtils LibraryHolder ${OpenCV_LIBS} fastdeploy_ppocr
    ONNXRuntime::ONNXRuntime HeaderOnlyLibraries)

# clang 15之后有ranges if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang") find_package(range-v3 REQUIRED)
# target_link_libraries(MaaFramework range-v3::range-v3) endif ()
add_dependencies(MaaFramework MaaUtils LibraryHolder)

if(WITH_ADB_CONTROLLER)
    add_dependencies(MaaFramework MaaAdbControlUnit)
endif()

if(WITH_WIN32_CONTROLLER)
    add_dependencies(MaaFramework MaaWin32ControlUnit)
endif()

if(WITH_THRIFT_CONTROLLER)
    add_dependencies(MaaFramework MaaThriftControlUnit)
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
install(DIRECTORY "${MAA_PUBLIC_INC}/MaaFramework" DESTINATION "include")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_framework_src})

if(WIN32)
    add_custom_command(
        TARGET MaaFramework
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:MaaFramework> $<TARGET_FILE_DIR:MaaFramework>
        COMMAND_EXPAND_LISTS)
endif()
