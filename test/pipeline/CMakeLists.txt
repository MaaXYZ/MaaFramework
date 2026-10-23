file(
    GLOB_RECURSE
    pipeline_testing_src
    *.cpp
    *.h
    *.hpp)

add_executable(PipelineTesting ${pipeline_testing_src})

target_link_libraries(PipelineTesting MaaFramework)

# BatchOcrRepro 用例的 OCR 模型构建时从 TestingDataSet 子模块复制（不入库，见 .gitignore）
set(batch_ocr_repro_model_dir ${CMAKE_CURRENT_SOURCE_DIR}/resources/BatchOcrRepro/resource/model/ocr)
add_custom_command(
    OUTPUT ${batch_ocr_repro_model_dir}/det.onnx
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/test/TestingDataSet/MaaCommonAssets/OCR/ppocr_v4/zh_cn
            ${batch_ocr_repro_model_dir}
    DEPENDS ${CMAKE_SOURCE_DIR}/test/TestingDataSet/MaaCommonAssets/OCR/ppocr_v4/zh_cn/det.onnx)
add_custom_target(BatchOcrReproResource ALL DEPENDS ${batch_ocr_repro_model_dir}/det.onnx)
set_target_properties(BatchOcrReproResource PROPERTIES FOLDER Testing)

add_dependencies(PipelineTesting MaaFramework PipelineSmokingResource BatchOcrReproResource)
set_target_properties(PipelineTesting PROPERTIES FOLDER Testing)

install(TARGETS PipelineTesting RUNTIME DESTINATION bin)
install(DIRECTORY resources/BatchOcrRepro DESTINATION test)

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:PipelineTesting> DESTINATION symbol OPTIONAL)
endif()
