file(
    GLOB_RECURSE
    pipeline_testing_src
    *.cpp
    *.h
    *.hpp)
    
add_executable(PipelineTesting ${pipeline_testing_src})

target_link_libraries(PipelineTesting MaaFramework)

add_dependencies(PipelineTesting MaaFramework PipelineSmokingResource)
set_target_properties(PipelineTesting PROPERTIES FOLDER Testing)

install(TARGETS PipelineTesting RUNTIME DESTINATION bin)
