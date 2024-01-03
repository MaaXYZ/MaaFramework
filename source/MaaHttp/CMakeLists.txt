set(LHG_EMBEDDED ON CACHE BOOL "make lhg work in embedded mode")
add_subdirectory(LHG)

file(GLOB_RECURSE maa_http_src wrapper.cpp LHG/projects/target/*.cpp LHG/projects/target/*.hpp LHG/projects/target/*.h)

add_executable(MaaHttp ${maa_http_src})
target_include_directories(MaaHttp PRIVATE ./../include LHG/projects/target)
target_link_libraries(MaaHttp HeaderOnlyLibraries Boost::system MaaFramework MaaToolkit)

if(APPLE)
  target_compile_definitions(MaaHttp PRIVATE LHG_USE_FMTLIB)
endif()

if(WIN32)
  target_compile_definitions(MaaHttp PRIVATE _WIN32_WINNT=0x0A00)
endif()

install(
  TARGETS MaaHttp
  EXPORT MaaFrameworkTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION bin
  ARCHIVE DESTINATION lib)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_http_src})
