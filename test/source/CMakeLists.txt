file(
    GLOB_RECURSE
    testing_src
    *.cpp
    *.h
    *.hpp)
    
add_executable(Testing ${testing_src})

target_link_libraries(Testing MaaFramework)

add_dependencies(Testing MaaFramework PipelineSmokingResource)

install(TARGETS Testing RUNTIME DESTINATION bin)
