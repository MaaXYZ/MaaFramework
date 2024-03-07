if(BUILD_XCFRAMEWORK)
    add_custom_command(
        OUTPUT MaaFramework.xcframework
        COMMAND rm -rf MaaFramework.xcframework
        COMMAND xcodebuild -create-xcframework -library libMaaFramework.dylib -headers ${PROJECT_SOURCE_DIR}/include
        -output MaaFramework.xcframework
        DEPENDS MaaFramework)

    add_custom_command(
        OUTPUT OpenCV.xcframework
        COMMAND rm -rf OpenCV.xcframework
        COMMAND
        xcodebuild -create-xcframework -library
        "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/libopencv_world4.408.dylib" -output
        OpenCV.xcframework)

    add_custom_command(
        OUTPUT ONNXRuntime.xcframework
        COMMAND rm -rf ONNXRuntime.xcframework
        COMMAND
        xcodebuild -create-xcframework -library
        "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/libonnxruntime.1.16.0.dylib" -output
        ONNXRuntime.xcframework)

    add_custom_command(
        OUTPUT fastdeploy_ppocr.xcframework
        COMMAND rm -rf fastdeploy_ppocr.xcframework
        COMMAND
        xcodebuild -create-xcframework -library
        "${MAADEPS_DIR}/runtime/${MAADEPS_TRIPLET}/libfastdeploy_ppocr.dylib" -output
        fastdeploy_ppocr.xcframework)

    add_custom_target(MaaXCFramework ALL DEPENDS MaaFramework MaaFramework.xcframework OpenCV.xcframework
        ONNXRuntime.xcframework fastdeploy_ppocr.xcframework)
endif(BUILD_XCFRAMEWORK)

target_compile_options(MaaFramework PRIVATE -Wno-deprecated-declarations -Wno-gnu-zero-variadic-macro-arguments)
