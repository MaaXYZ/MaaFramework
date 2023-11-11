file(GLOB_RECURSE maa_framework_src *.h *.hpp *.cpp)
file(GLOB_RECURSE maa_framework_header ../../include/MaaFramework/*)

add_library(MaaFramework SHARED ${maa_framework_src} ${maa_framework_header})

set(MaaFramework_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaFramework)

target_include_directories(
    MaaFramework
    INTERFACE ../../include
    PRIVATE . ../include ../../include)

target_compile_definitions(MaaFramework PRIVATE MAA_FRAMEWORK_EXPORTS)

target_link_libraries(MaaFramework MaaUtils ${OpenCV_LIBS} fastdeploy_ppocr ONNXRuntime::ONNXRuntime HeaderOnlyLibraries)

# clang 15之后有ranges if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang") find_package(range-v3 REQUIRED)
# target_link_libraries(MaaFramework range-v3::range-v3) endif ()

add_dependencies(MaaFramework MaaUtils)

if (WITH_ADB_CONTROLLER)
    add_dependencies(MaaFramework MaaAdbControlUnit)
endif(WITH_ADB_CONTROLLER)
if(WITH_THRIFT_CONTROLLER AND NOT MAA_CROSSCOMPILE)
    add_dependencies(MaaFramework MaaThriftControlUnit)
endif(WITH_THRIFT_CONTROLLER AND NOT MAA_CROSSCOMPILE)
if (WITH_DBG_CONTROLLER)
    add_dependencies(MaaFramework MaaDbgControlUnit)
endif(WITH_DBG_CONTROLLER)

install(
    TARGETS MaaFramework
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaFramework" DESTINATION "include")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../include/Interface" DESTINATION "share")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_framework_src})

if(WIN32)
    add_custom_command(
        TARGET MaaFramework
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:MaaFramework> $<TARGET_FILE_DIR:MaaFramework>
        COMMAND_EXPAND_LISTS)
endif(WIN32)
