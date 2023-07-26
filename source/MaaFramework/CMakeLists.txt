file(GLOB_RECURSE maa_src *.cpp *.h *.hpp ../include/*.h)

add_library(MaaFramework SHARED ${maa_src})

file(GLOB MaaFramework_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.h)
set(MaaFramework_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_include_directories(MaaFramework PUBLIC ../../include PRIVATE . ../include ${MaaThriftController_INCLUDE_DIRS})
set_target_properties(MaaFramework PROPERTIES
    PUBLIC_HEADER "${MaaFramework_PUBLIC_HEADERS}"
)
target_compile_definitions(MaaFramework PRIVATE MAA_DLL_EXPORTS)

target_link_libraries(MaaFramework MaaControlUnit MaaUtils)
if(WITH_THRIFT)
    target_link_libraries(MaaFramework MaaThriftController)
endif(WITH_THRIFT)
target_link_libraries(MaaFramework ${OpenCV_LIBS} MaaDerpLearning ONNXRuntime::ONNXRuntime HeaderOnlyLibraries) #asio::asio cpr::cpr

# clang 15之后有ranges
# if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
#     find_package(range-v3 REQUIRED)
#     target_link_libraries(MaaFramework range-v3::range-v3)
# endif ()

add_dependencies(MaaFramework MaaControlUnit MaaUtils MaaThriftController)

install(TARGETS MaaFramework
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    PUBLIC_HEADER DESTINATION include
    ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${maa_src})

if (WIN32)
  add_custom_command(TARGET MaaFramework POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:MaaFramework> $<TARGET_FILE_DIR:MaaFramework>
    COMMAND_EXPAND_LISTS
  )
endif (WIN32)
