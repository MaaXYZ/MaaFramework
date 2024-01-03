set(LHG_EMBEDDED ON CACHE BOOL "make lhg work in embedded mode")
add_subdirectory(LHG)

file(GLOB_RECURSE maa_http_src wrapper.cpp LHG/projects/target/*.cpp LHG/projects/target/*.hpp LHG/projects/target/*.h)

add_executable(MaaHttp ${maa_http_src})
target_include_directories(MaaHttp PRIVATE ./../include LHG/projects/target)
target_link_libraries(MaaHttp LHGHeaderonly Boost::system
                      MaaFramework MaaToolkit)
