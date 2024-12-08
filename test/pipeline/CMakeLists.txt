file(
    GLOB_RECURSE
    pipeline_testing_src
    *.cpp
    *.h
    *.hpp)

add_executable(PipelineTesting ${pipeline_testing_src})

target_link_libraries(PipelineTesting MaaFramework HeaderOnlyLibraries)

add_dependencies(PipelineTesting MaaFramework PipelineSmokingResource)
set_target_properties(PipelineTesting PROPERTIES FOLDER Testing)

install(TARGETS PipelineTesting RUNTIME DESTINATION bin)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:PipelineTesting> DESTINATION symbol OPTIONAL)
endif()
