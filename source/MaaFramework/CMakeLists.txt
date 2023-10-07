file(GLOB_RECURSE maa_src *.cpp *.h *.hpp ../include ../../include/MaaFramework)

add_library(MaaFramework SHARED ${maa_src})

set(MaaFramework_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaFramework)

target_include_directories(MaaFramework INTERFACE ../../include PRIVATE .  ../include ../../include ${MaaThriftController_INCLUDE_DIRS})

target_compile_definitions(MaaFramework PRIVATE MAA_FRAMEWORK_EXPORTS)

target_link_libraries(MaaFramework MaaControlUnit MaaUtils)
if(WITH_THRIFT)
    target_link_libraries(MaaFramework MaaThriftController)
endif(WITH_THRIFT)
target_link_libraries(MaaFramework ${OpenCV_LIBS} MaaDerpLearning ONNXRuntime::ONNXRuntime HeaderOnlyLibraries)

# clang 15之后有ranges
# if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
#     find_package(range-v3 REQUIRED)
#     target_link_libraries(MaaFramework range-v3::range-v3)
# endif ()

add_dependencies(MaaFramework MaaControlUnit MaaUtils)
if(WITH_THRIFT)
    add_dependencies(MaaFramework MaaThriftController)
endif(WITH_THRIFT)
install(TARGETS MaaFramework
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../include/MaaFramework" DESTINATION "include")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../include/Interface" DESTINATION "share")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_src})

if (WIN32)
  add_custom_command(TARGET MaaFramework POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:MaaFramework> $<TARGET_FILE_DIR:MaaFramework>
    COMMAND_EXPAND_LISTS
  )
endif (WIN32)
