#include "NeuralNetworkCache.h"

#include <fstream>
#include <iostream>

#include "Global/OptionMgr.h"
#include "Vision/NeuralNetworkCache.h"

namespace
{
using namespace MAA_VISION_NS;

size_t count_runs(Ort::Session& session)
{
    Ort::AllocatorWithDefaultOptions allocator;
    auto profile_path = session.EndProfilingAllocated(allocator);
    std::ifstream profile(profile_path.get());
    std::string text((std::istreambuf_iterator<char>(profile)), std::istreambuf_iterator<char>());
    auto events = json::parse(text);
    if (!events || !events->is_array()) {
        return SIZE_MAX;
    }
    size_t runs = 0;
    for (const auto& event : events->as_array()) {
        if (event.get("name", std::string()) == "model_run") {
            ++runs;
        }
    }
    return runs;
}

bool test_classifier(const std::filesystem::path& model_dir, const std::filesystem::path& log_dir)
{
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "cache-test");
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    options.EnableProfiling((log_dir / "classifier-cached").c_str());
    auto session = std::make_shared<Ort::Session>(env, (model_dir / "classifier.onnx").c_str(), options);
    const auto memory = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    const cv::Mat image(32, 64, CV_8UC3, cv::Scalar(255, 255, 255));
    const cv::Rect roi(0, 0, 32, 32);
    InferenceCache<NeuralNetworkClassifierResult> cache;

    NeuralNetworkClassifierParam param;
    param.labels = { "white", "black" };
    param.expected = { 1 };
    NeuralNetworkClassifier miss(image, { roi }, param, session, memory, "miss", &cache);
    if (miss.best_result() || miss.all_results().size() != 1) {
        return false;
    }

    param.labels = { "renamed", "other" };
    param.expected = { std::string("renamed") };
    param.result_index = -1;
    for (int i = 0; i < 31; ++i) {
        NeuralNetworkClassifier hit(image, { roi }, param, session, memory, "hit", &cache);
        if (!hit.best_result() || hit.best_result()->label != "renamed" || hit.best_result()->cls_index != 0
            || hit.best_result()->raw != miss.all_results().front().raw || hit.best_result()->probs != miss.all_results().front().probs
            || hit.draws().size() != 1) {
            return false;
        }
    }

    // Coordinates matter even when the crops have identical pixels.
    const cv::Rect other_roi(32, 0, 32, 32);
    NeuralNetworkClassifier shifted(image, { other_roi }, param, session, memory, "shifted", &cache);
    if (!shifted.best_result() || shifted.best_result()->box != other_roi) {
        return false;
    }
    NeuralNetworkClassifier multi(image, { roi, other_roi }, param, session, memory, "multi-roi", &cache);
    if (multi.all_results().size() != 2 || !multi.best_result() || multi.best_result()->box != other_roi || multi.draws().size() != 2) {
        return false;
    }
    if (count_runs(*session) != 2) {
        return false;
    }

    options.EnableProfiling((log_dir / "classifier-uncached").c_str());
    auto uncached_session = std::make_shared<Ort::Session>(env, (model_dir / "classifier.onnx").c_str(), options);
    for (int i = 0; i < 32; ++i) {
        NeuralNetworkClassifier plain(image, { roi }, param, uncached_session, memory, "plain");
        if (!plain.best_result() || plain.best_result()->raw != miss.all_results().front().raw) {
            return false;
        }
    }
    if (count_runs(*uncached_session) != 32) {
        return false;
    }

    // A fresh session cannot reuse predictions from the previous model session.
    options.EnableProfiling((log_dir / "classifier-reloaded").c_str());
    auto reloaded = std::make_shared<Ort::Session>(env, (model_dir / "classifier.onnx").c_str(), options);
    const cv::Mat black(32, 64, CV_8UC3, cv::Scalar(0, 0, 0));
    param.expected = { 1 };
    NeuralNetworkClassifier different_session(black, { roi }, param, reloaded, memory, "reloaded", &cache);
    if (!different_session.best_result() || different_session.best_result()->cls_index != 1 || count_runs(*reloaded) != 1) {
        return false;
    }
    std::cout << "Classifier: 32 requests, 32 uncached runs / 1 cached run; ROI and session isolation passed\n";
    return true;
}

bool test_detector(const std::filesystem::path& model_dir, const std::filesystem::path& log_dir)
{
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "cache-test");
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    options.EnableProfiling((log_dir / "detector-cached").c_str());
    auto session = std::make_shared<Ort::Session>(env, (model_dir / "detector.onnx").c_str(), options);
    const auto memory = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    const cv::Mat image(32, 32, CV_8UC3, cv::Scalar(255, 255, 255));
    const cv::Rect roi(0, 0, 32, 32);
    InferenceCache<NeuralNetworkDetectorResult> cache;

    NeuralNetworkDetectorParam param;
    param.labels = { "first", "second" };
    param.expected = { 0 };
    param.thresholds = { 0.95 };
    NeuralNetworkDetector miss(image, { roi }, param, session, memory, "high-threshold", &cache);
    if (miss.best_result() || miss.all_results().size() != 2) {
        return false;
    }

    param.labels = { "renamed-first", "renamed-second" };
    param.expected = { std::string("renamed-second") };
    param.thresholds = { 0.5 };
    param.order_by = ResultOrderBy::Score;
    param.result_index = -1;
    for (int i = 0; i < 31; ++i) {
        NeuralNetworkDetector hit(image, { roi }, param, session, memory, "hit", &cache);
        if (!hit.best_result() || hit.best_result()->label != "renamed-second" || hit.best_result()->cls_index != 1
            || hit.best_result()->box != cv::Rect(20, 20, 8, 8) || hit.draws().size() != 1) {
            return false;
        }
    }
    if (count_runs(*session) != 1) {
        return false;
    }

    options.EnableProfiling((log_dir / "detector-uncached").c_str());
    auto uncached_session = std::make_shared<Ort::Session>(env, (model_dir / "detector.onnx").c_str(), options);
    for (int i = 0; i < 32; ++i) {
        NeuralNetworkDetector plain(image, { roi }, param, uncached_session, memory, "plain");
        if (!plain.best_result() || plain.best_result()->box != cv::Rect(20, 20, 8, 8)
            || plain.all_results().size() != miss.all_results().size()) {
            return false;
        }
    }
    if (count_runs(*uncached_session) != 32) {
        return false;
    }

    // Empty detections are successful inference results and should be reused.
    options.EnableProfiling((log_dir / "detector-empty").c_str());
    auto empty_session = std::make_shared<Ort::Session>(env, (model_dir / "detector.onnx").c_str(), options);
    const cv::Mat black(32, 32, CV_8UC3, cv::Scalar(0, 0, 0));
    for (int i = 0; i < 2; ++i) {
        NeuralNetworkDetector empty(black, { roi }, param, empty_session, memory, "empty", &cache);
        if (!empty.all_results().empty() || empty.best_result()) {
            return false;
        }
    }
    if (count_runs(*empty_session) != 1) {
        return false;
    }
    std::cout << "Detector: 32 requests, 32 uncached runs / 1 cached run; independent filters and empty results passed\n";
    return true;
}
}

bool test_neural_network_cache(const std::filesystem::path& testset_dir, const std::filesystem::path& log_dir)
{
    std::filesystem::create_directories(log_dir);
    bool debug_draw = true;
    auto& options = MAA_GLOBAL_NS::OptionMgr::get_instance();
    const bool previous_debug = options.debug_mode();
    options.set_option(MaaGlobalOption_DebugMode, &debug_draw, sizeof(debug_draw));
    const auto model_dir = testset_dir / "NeuralNetworkCache";
    const bool passed = test_classifier(model_dir, log_dir) && test_detector(model_dir, log_dir);
    debug_draw = previous_debug;
    options.set_option(MaaGlobalOption_DebugMode, &debug_draw, sizeof(debug_draw));
    if (!passed) {
        std::cerr << "Neural network cache regression failed\n";
        return false;
    }
    return true;
}
