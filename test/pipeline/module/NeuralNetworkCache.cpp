#include "NeuralNetworkCache.h"

#include <iostream>

#include "Global/OptionMgr.h"
#include "Resource/OrtSession.h"
#include "Vision/NeuralNetworkCache.h"

namespace
{
using namespace MAA_VISION_NS;

class CountingSession : public InferenceSession
{
public:
    explicit CountingSession(InferenceSessionPtr session)
        : session_(std::move(session))
    {
    }

    InferenceEngine engine() const override { return session_->engine(); }

    const std::vector<int64_t>& input_shape() const override { return session_->input_shape(); }

    const std::unordered_map<std::string, std::string>& metadata() const override { return session_->metadata(); }

    std::optional<InferenceOutput> run(const std::vector<float>& input) const override
    {
        ++runs;
        if (fail_next) {
            fail_next = false;
            return std::nullopt;
        }
        return session_->run(input);
    }

    mutable size_t runs = 0;
    mutable bool fail_next = false;

private:
    InferenceSessionPtr session_;
};

std::shared_ptr<CountingSession> make_session(Ort::Env& env, const std::filesystem::path& model, const Ort::SessionOptions& options)
{
    const auto memory = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    auto session = MAA_RES_NS::OrtSession::create(env, model, options, memory);
    return session ? std::make_shared<CountingSession>(std::move(session)) : nullptr;
}

bool test_classifier(const std::filesystem::path& model_dir)
{
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "cache-test");
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    auto session = make_session(env, model_dir / "classifier.onnx", options);
    if (!session) {
        return false;
    }
    const cv::Mat image(32, 64, CV_8UC3, cv::Scalar(255, 255, 255));
    const cv::Rect roi(0, 0, 32, 32);
    InferenceCache<NeuralNetworkClassifierResult> cache;

    NeuralNetworkClassifierParam param;
    param.labels = { "white", "black" };
    param.expected = { 1 };
    NeuralNetworkClassifier miss(image, { roi }, param, session, "miss", &cache);
    if (miss.best_result() || miss.all_results().size() != 1) {
        return false;
    }

    param.labels = { "renamed", "other" };
    param.expected = { std::string("renamed") };
    param.result_index = -1;
    for (int i = 0; i < 31; ++i) {
        NeuralNetworkClassifier hit(image, { roi }, param, session, "hit", &cache);
        if (!hit.best_result() || hit.best_result()->label != "renamed" || hit.best_result()->cls_index != 0
            || hit.best_result()->raw != miss.all_results().front().raw || hit.best_result()->probs != miss.all_results().front().probs
            || hit.draws().size() != 1) {
            return false;
        }
    }

    // Coordinates matter even when the crops have identical pixels.
    const cv::Rect other_roi(32, 0, 32, 32);
    NeuralNetworkClassifier shifted(image, { other_roi }, param, session, "shifted", &cache);
    if (!shifted.best_result() || shifted.best_result()->box != other_roi) {
        return false;
    }
    NeuralNetworkClassifier multi(image, { roi, other_roi }, param, session, "multi-roi", &cache);
    if (multi.all_results().size() != 2 || !multi.best_result() || multi.best_result()->box != other_roi || multi.draws().size() != 2) {
        return false;
    }
    if (session->runs != 2) {
        return false;
    }

    auto uncached_session = make_session(env, model_dir / "classifier.onnx", options);
    if (!uncached_session) {
        return false;
    }
    for (int i = 0; i < 32; ++i) {
        NeuralNetworkClassifier plain(image, { roi }, param, uncached_session, "plain");
        if (!plain.best_result() || plain.best_result()->raw != miss.all_results().front().raw) {
            return false;
        }
    }
    if (uncached_session->runs != 32) {
        return false;
    }

    // A fresh session cannot reuse predictions from the previous model session.
    auto reloaded = make_session(env, model_dir / "classifier.onnx", options);
    if (!reloaded) {
        return false;
    }
    const cv::Mat black(32, 64, CV_8UC3, cv::Scalar(0, 0, 0));
    param.expected = { 1 };
    NeuralNetworkClassifier different_session(black, { roi }, param, reloaded, "reloaded", &cache);
    if (!different_session.best_result() || different_session.best_result()->cls_index != 1 || reloaded->runs != 1) {
        return false;
    }
    // A transient inference failure must not poison the cache.
    auto retry_session = make_session(env, model_dir / "classifier.onnx", options);
    if (!retry_session) {
        return false;
    }
    retry_session->fail_next = true;
    param.expected = { 0 };
    NeuralNetworkClassifier failed(image, { roi }, param, retry_session, "failed", &cache);
    NeuralNetworkClassifier retried(image, { roi }, param, retry_session, "retried", &cache);
    NeuralNetworkClassifier reused(image, { roi }, param, retry_session, "reused", &cache);
    if (failed.best_result() || !retried.best_result() || !reused.best_result() || retry_session->runs != 2) {
        return false;
    }
    std::cout << "Classifier: 32 requests, 32 uncached runs / 1 cached run; ROI and session isolation passed\n";
    return true;
}

bool test_detector(const std::filesystem::path& model_dir)
{
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "cache-test");
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    auto session = make_session(env, model_dir / "detector.onnx", options);
    if (!session) {
        return false;
    }
    const cv::Mat image(32, 32, CV_8UC3, cv::Scalar(255, 255, 255));
    const cv::Rect roi(0, 0, 32, 32);
    InferenceCache<NeuralNetworkDetectorResult> cache;

    NeuralNetworkDetectorParam param;
    param.labels = { "first", "second" };
    param.expected = { 0 };
    param.thresholds = { 0.95 };
    NeuralNetworkDetector miss(image, { roi }, param, session, "high-threshold", &cache);
    if (miss.best_result() || miss.all_results().size() != 2) {
        return false;
    }

    param.labels = { "renamed-first", "renamed-second" };
    param.expected = { std::string("renamed-second") };
    param.thresholds = { 0.5 };
    param.order_by = ResultOrderBy::Score;
    param.result_index = -1;
    for (int i = 0; i < 31; ++i) {
        NeuralNetworkDetector hit(image, { roi }, param, session, "hit", &cache);
        if (!hit.best_result() || hit.best_result()->label != "renamed-second" || hit.best_result()->cls_index != 1
            || hit.best_result()->box != cv::Rect(20, 20, 8, 8) || hit.draws().size() != 1) {
            return false;
        }
    }
    if (session->runs != 1) {
        return false;
    }

    auto uncached_session = make_session(env, model_dir / "detector.onnx", options);
    if (!uncached_session) {
        return false;
    }
    for (int i = 0; i < 32; ++i) {
        NeuralNetworkDetector plain(image, { roi }, param, uncached_session, "plain");
        if (!plain.best_result() || plain.best_result()->box != cv::Rect(20, 20, 8, 8)
            || plain.all_results().size() != miss.all_results().size()) {
            return false;
        }
    }
    if (uncached_session->runs != 32) {
        return false;
    }

    // Empty detections are successful inference results and should be reused.
    auto empty_session = make_session(env, model_dir / "detector.onnx", options);
    if (!empty_session) {
        return false;
    }
    const cv::Mat black(32, 32, CV_8UC3, cv::Scalar(0, 0, 0));
    for (int i = 0; i < 2; ++i) {
        NeuralNetworkDetector empty(black, { roi }, param, empty_session, "empty", &cache);
        if (!empty.all_results().empty() || empty.best_result()) {
            return false;
        }
    }
    if (empty_session->runs != 1) {
        return false;
    }
    auto retry_session = make_session(env, model_dir / "detector.onnx", options);
    if (!retry_session) {
        return false;
    }
    retry_session->fail_next = true;
    NeuralNetworkDetector failed(image, { roi }, param, retry_session, "failed", &cache);
    NeuralNetworkDetector retried(image, { roi }, param, retry_session, "retried", &cache);
    NeuralNetworkDetector reused(image, { roi }, param, retry_session, "reused", &cache);
    if (!failed.all_results().empty() || !retried.best_result() || !reused.best_result() || retry_session->runs != 2) {
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
    const bool passed = test_classifier(model_dir) && test_detector(model_dir);
    debug_draw = previous_debug;
    options.set_option(MaaGlobalOption_DebugMode, &debug_draw, sizeof(debug_draw));
    if (!passed) {
        std::cerr << "Neural network cache regression failed\n";
        return false;
    }
    return true;
}
