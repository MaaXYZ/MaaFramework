file(
    GLOB_RECURSE
    pipeline_testing_src
    *.cpp
    *.h
    *.hpp)

add_executable(PipelineTesting ${pipeline_testing_src})

target_link_libraries(PipelineTesting MaaFramework)

# Exercise the private vision classes with ORT profiling, without exporting
# internal APIs from MaaFramework just for tests.
target_sources(PipelineTesting PRIVATE
    ../../source/MaaFramework/Vision/NeuralNetworkClassifier.cpp
    ../../source/MaaFramework/Vision/NeuralNetworkDetector.cpp
    ../../source/MaaFramework/Vision/VisionBase.cpp
    ../../source/MaaFramework/Global/OptionMgr.cpp)
target_include_directories(PipelineTesting PRIVATE ../../source/MaaFramework ${MAA_PRIVATE_INC})
target_link_libraries(PipelineTesting MaaUtils ${OpenCV_LIBS} ONNXRuntime::ONNXRuntime Boost::regex HeaderOnlyLibraries)

add_dependencies(PipelineTesting MaaFramework PipelineSmokingResource)
set_target_properties(PipelineTesting PROPERTIES FOLDER Testing)

install(TARGETS PipelineTesting RUNTIME DESTINATION bin)
install(DIRECTORY resource/NeuralNetworkCache DESTINATION test)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:PipelineTesting> DESTINATION symbol OPTIONAL)
endif()
