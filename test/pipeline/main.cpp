#include <filesystem>
#include <iostream>
#include <string_view>

#include "module/ControllerActionEvents.h"
#include "module/InferenceContext.h"
#include "module/NeuralNetworkCache.h"
#include "module/OCRCache.h"
#include "module/OCRRegression.h"
#include "module/PipelineSmoking.h"
#include "module/PostStopInterrupt.h"
#include "module/RunWithoutFile.h"
#include "module/RuntimeCache.h"
#include "module/RuntimeEvents.h"

#include "MaaFramework/MaaAPI.h"

int main([[maybe_unused]] int argc, char** argv)
{
    auto cur_dir = std::filesystem::path(argv[0]).parent_path();
    auto testset_dir = cur_dir.parent_path() / "test";
    const bool ocr_only = argc >= 2 && std::string_view(argv[1]) == "--ocr-regression";
    if (ocr_only && argc != 3 && argc != 4) {
        std::cerr << "Usage: PipelineTesting --ocr-regression <testset_dir> [report_dir]\n";
        return 1;
    }
    if (ocr_only) {
        testset_dir = argv[2];
    }
    else if (argc == 2) {
        testset_dir = argv[1];
    }

    std::string logging_dir = (cur_dir / "debug").string();
    MaaGlobalSetOption(MaaGlobalOption_LogDir, static_cast<void*>(logging_dir.data()), logging_dir.size());
    bool on = true;
    MaaGlobalSetOption(MaaGlobalOption_SaveDraw, &on, sizeof(on));
    MaaLoggingLevel lv = MaaLoggingLevel_Info;
    MaaGlobalSetOption(MaaGlobalOption_StdoutLevel, &lv, sizeof(lv));

    if (ocr_only) {
        const auto report_dir = argc == 4 ? std::filesystem::path(argv[3]) : cur_dir / "debug" / "ocr-regression";
        return ocr_regression(testset_dir, report_dir) ? 0 : 1;
    }

    if (!runtime_events()) {
        return -1;
    }

    if (!test_runtime_cache()) {
        return -1;
    }

    if (!test_inference_context(testset_dir)) {
        return -1;
    }

    if (!test_neural_network_cache(testset_dir, cur_dir / "debug")) {
        return -1;
    }

    if (!test_ocr_cache()) {
        return -1;
    }

    if (!run_without_file(testset_dir)) {
        return -1;
    }
    if (!pipeline_smoking(testset_dir)) {
        return -1;
    }
    if (!post_stop_interrupt(testset_dir)) {
        return -1;
    }
    if (!controller_action_events()) {
        return -1;
    }

    return 0;
}
