file(
    GLOB_RECURSE
    pipeline_testing_src
    *.cpp
    *.h
    *.hpp)

add_executable(PipelineTesting ${pipeline_testing_src})

target_link_libraries(PipelineTesting MaaFramework)

# Exercise the private framework classes and real inference sessions without exporting
# internal APIs from MaaFramework just for tests.
target_sources(PipelineTesting PRIVATE
    ../../source/MaaFramework/Resource/FastDeployOCRSession.cpp
    ../../source/MaaFramework/Resource/InferenceResMgr.cpp
    ../../source/MaaFramework/Resource/OrtSession.cpp
    ../../source/MaaFramework/Tasker/RuntimeCache.cpp
    ../../source/MaaFramework/Vision/NeuralNetworkClassifier.cpp
    ../../source/MaaFramework/Vision/NeuralNetworkDetector.cpp
    ../../source/MaaFramework/Vision/OCRer.cpp
    ../../source/MaaFramework/Vision/VisionBase.cpp
    ../../source/MaaFramework/Global/OptionMgr.cpp)
target_include_directories(PipelineTesting PRIVATE ../../source/MaaFramework ${MAA_PRIVATE_INC})
target_link_libraries(PipelineTesting MaaUtils ${OpenCV_LIBS} fastdeploy_ppocr ONNXRuntime::ONNXRuntime Boost::regex HeaderOnlyLibraries)

# Stage the issue corpus and models in the build tree, leaving the data submodule clean.
set(ocr_regression_dir ${CMAKE_CURRENT_BINARY_DIR}/OCRRegression)
add_custom_target(OCRRegressionResource ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/test/TestingDataSet/OCRRegression ${ocr_regression_dir}
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/test/TestingDataSet/MaaCommonAssets/OCR/ppocr_v4/zh_cn
            ${ocr_regression_dir}/resource/model/ocr)
set_target_properties(OCRRegressionResource PROPERTIES FOLDER Testing)

add_dependencies(PipelineTesting MaaFramework PipelineSmokingResource OCRRegressionResource)
set_target_properties(PipelineTesting PROPERTIES FOLDER Testing)

install(TARGETS PipelineTesting RUNTIME DESTINATION bin)
install(DIRECTORY resource/NeuralNetworkCache DESTINATION test)
install(DIRECTORY ${ocr_regression_dir} DESTINATION test)
install(DIRECTORY resources/PostStopInterrupt DESTINATION test)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:PipelineTesting> DESTINATION symbol OPTIONAL)
endif()
