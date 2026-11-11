#include "InferenceContext.h"

#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <numeric>

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
#endif

#include "Resource/FastDeployOCRSession.h"
#include "Resource/InferenceContext.h"
#include "Resource/InferenceResMgr.h"
#include "Resource/OrtSession.h"

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
MAA_SUPPRESS_CV_WARNINGS_END

namespace
{
using namespace std::chrono_literals;

bool blocks_while_locked(const std::function<bool()>& operation)
{
    auto lock = MAA_RES_NS::InferenceContext(true).lock();
    std::promise<void> started;
    auto result = std::async(std::launch::async, [&] {
        started.set_value();
        return operation();
    });
    started.get_future().wait();
    const bool blocked = result.wait_for(50ms) == std::future_status::timeout;
    lock.unlock();
    return result.get() && blocked;
}

bool completes_while_locked(const std::function<bool()>& operation)
{
    auto lock = MAA_RES_NS::InferenceContext(true).lock();
    auto result = std::async(std::launch::async, operation);
    const bool completed = result.wait_for(5s) == std::future_status::ready;
    lock.unlock();
    return result.get() && completed;
}

bool test_ort_context(const std::filesystem::path& testset_dir)
{
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "inference-context-test");
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    const auto memory = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    const auto model = testset_dir / "NeuralNetworkCache" / "classifier.onnx";

    // CPU models exercise the synchronization boundaries without requiring a GPU.
    MAA_RES_NS::InferenceContext context(true);
    std::shared_ptr<MAA_RES_NS::OrtSession> session;
    if (!blocks_while_locked([&] {
            session = MAA_RES_NS::OrtSession::create(env, model, options, memory, context);
            return session != nullptr;
        })) {
        std::cerr << "GPU session creation did not wait for the shared lock\n";
        return false;
    }

    // Reconfiguring a resource must not change an existing session's policy.
    context = { };
    const auto& shape = session->input_shape();
    const auto size = std::accumulate(shape.begin(), shape.end(), int64_t { 1 }, std::multiplies<int64_t> { });
    const std::vector<float> input(size, 1.0f);
    if (!blocks_while_locked([&] { return session->run(input).has_value(); })) {
        std::cerr << "GPU inference did not retain its original context\n";
        return false;
    }

    if (!completes_while_locked([&] {
            auto cpu = MAA_RES_NS::OrtSession::create(env, model, options, memory);
            return cpu && cpu->run(input).has_value();
        })) {
        std::cerr << "CPU session lifecycle was blocked by GPU operations\n";
        return false;
    }

    auto last_reference = session;
    if (!completes_while_locked([&] {
            session.reset();
            return true;
        })) {
        std::cerr << "Releasing a non-final model reference was blocked\n";
        return false;
    }

    if (!blocks_while_locked([&] {
            last_reference.reset();
            return true;
        })) {
        std::cerr << "GPU session destruction did not retain its original context\n";
        return false;
    }

    // Resource cleanup can release the last model reference while already locked.
    auto lock = MAA_RES_NS::InferenceContext(true).lock();
    context = MAA_RES_NS::InferenceContext(true);
    session = MAA_RES_NS::OrtSession::create(env, model, options, memory, context);
    if (!session) {
        return false;
    }
    session.reset();
    if (MAA_RES_NS::OrtSession::create(env, model / "missing.onnx", options, memory, context)) {
        std::cerr << "Loading a missing model unexpectedly succeeded\n";
        return false;
    }
    return true;
}

// Only the advertised device changes; these methods never invoke a GPU runtime.
class StubRecognizer final : public fastdeploy::vision::ocr::Recognizer
{
public:
    explicit StubRecognizer(fastdeploy::Device device) { runtime_option.device = device; }

    bool Predict(const cv::Mat&, std::string* text, float* score) override
    {
        *text = "test";
        *score = 1.0f;
        return true;
    }

    bool BatchPredict(const std::vector<cv::Mat>& images, fastdeploy::vision::OCRResult* result) override
    {
        result->text.assign(images.size(), "test");
        result->rec_scores.assign(images.size(), 1.0f);
        return true;
    }
};

bool test_ocr_context()
{
    const cv::Mat image(16, 16, CV_8UC3, cv::Scalar(255, 255, 255));
    for (const auto device :
         { fastdeploy::Device::CUDA, fastdeploy::Device::DIRECTML, fastdeploy::Device::WEBGPU, fastdeploy::Device::COREML }) {
        const MAA_RES_NS::InferenceContext context(true);
        std::shared_ptr<StubRecognizer> model;
        if (!blocks_while_locked([&] {
                model = context.make_shared<StubRecognizer>(device);
                return model != nullptr;
            })) {
            std::cerr << "OCR model creation did not wait for GPU operations\n";
            return false;
        }
        auto session = std::make_shared<MAA_RES_NS::FastDeployOCRSession>(nullptr, model, nullptr);
        // The session captures the device, independent of later option changes.
        model->runtime_option.UseCpu();
        model.reset();

        if (!blocks_while_locked([&] { return session->rec(image).has_value(); })
            || !blocks_while_locked([&] { return session->batch_rec({ image, image }).has_value(); })
            || !blocks_while_locked([&] { return !session->det_and_rec(image).has_value(); }) || !blocks_while_locked([&] {
                   session.reset();
                   return true;
               })) {
            std::cerr << "OCR operations did not retain the shared GPU context for device " << static_cast<int>(device) << '\n';
            return false;
        }
    }

    if (!completes_while_locked([&] {
            const MAA_RES_NS::InferenceContext context;
            auto model = context.make_shared<StubRecognizer>(fastdeploy::Device::CPU);
            MAA_RES_NS::FastDeployOCRSession session(nullptr, std::move(model), nullptr);
            return session.rec(image).has_value() && session.batch_rec({ image }).has_value() && !session.det_and_rec(image).has_value();
        })) {
        std::cerr << "CPU OCR operations were blocked by GPU operations\n";
        return false;
    }
    return true;
}

bool test_provider_context(const std::filesystem::path& testset_dir)
{
    MAA_RES_NS::InferenceResMgr manager;
    bool configured = false;
    // Even a failed GPU configuration must wait for other GPU operations.
    if (!blocks_while_locked([&] {
            configured = manager.use_cuda(0);
            return true;
        })) {
        std::cerr << "GPU provider configuration did not wait for the shared lock\n";
        return false;
    }

    if (configured && !blocks_while_locked([&] { return manager.use_cpu(); })) {
        std::cerr << "Switching away from a GPU provider did not wait for the shared lock\n";
        return false;
    }

    manager.lazy_load_classifier(testset_dir / "NeuralNetworkCache");
    if (!completes_while_locked([&] { return manager.classifier("classifier.onnx") != nullptr; })) {
        std::cerr << "CPU model loading was blocked after a failed or reset GPU provider\n";
        return false;
    }
    return true;
}
}

bool test_inference_context(const std::filesystem::path& testset_dir)
{
    auto unlocked = MAA_RES_NS::InferenceContext().lock();
    if (unlocked.mutex() != nullptr || unlocked.owns_lock()) {
        std::cerr << "Disabled inference synchronization did not return an empty lock\n";
        return false;
    }

    if (!test_ort_context(testset_dir) || !test_ocr_context() || !test_provider_context(testset_dir)) {
        return false;
    }
    std::cout << "GPU inference context: shared lifecycle lock, backend snapshots and CPU bypass passed\n";
    return true;
}
