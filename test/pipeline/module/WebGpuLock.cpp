#include "WebGpuLock.h"

#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <numeric>

#include "Resource/OrtSession.h"
#include "Resource/WebGpuLock.h"

namespace
{
using namespace std::chrono_literals;

bool blocks_while_locked(const std::function<bool()>& operation)
{
    auto lock = MAA_RES_NS::lock_webgpu(true);
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
    auto lock = MAA_RES_NS::lock_webgpu(true);
    auto result = std::async(std::launch::async, operation);
    const bool completed = result.wait_for(5s) == std::future_status::ready;
    lock.unlock();
    return result.get() && completed;
}
}

bool test_webgpu_lock(const std::filesystem::path& testset_dir)
{
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "webgpu-lock-test");
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    const auto memory = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    const auto model = testset_dir / "NeuralNetworkCache" / "classifier.onnx";

    // CPU models exercise the synchronization boundaries without requiring a GPU.
    std::shared_ptr<MAA_RES_NS::OrtSession> session;
    if (!blocks_while_locked([&] {
            session = MAA_RES_NS::OrtSession::create(env, model, options, memory, true);
            return session != nullptr;
        })) {
        std::cerr << "WebGPU session creation did not wait for the shared lock\n";
        return false;
    }

    const auto& shape = session->input_shape();
    const auto size = std::accumulate(shape.begin(), shape.end(), int64_t { 1 }, std::multiplies<int64_t> {});
    const std::vector<float> input(size, 1.0f);
    if (!blocks_while_locked([&] { return session->run(input).has_value(); })) {
        std::cerr << "WebGPU inference did not wait for the shared lock\n";
        return false;
    }

    if (!completes_while_locked([&] {
            auto cpu = MAA_RES_NS::OrtSession::create(env, model, options, memory);
            return cpu && cpu->run(input).has_value();
        })) {
        std::cerr << "CPU session lifecycle was blocked by the WebGPU lock\n";
        return false;
    }

    if (!blocks_while_locked([&] {
            session.reset();
            return true;
        })) {
        std::cerr << "WebGPU session destruction did not wait for the shared lock\n";
        return false;
    }

    // Resource cleanup can release the last model reference while already locked.
    auto lock = MAA_RES_NS::lock_webgpu(true);
    session = MAA_RES_NS::OrtSession::create(env, model, options, memory, true);
    if (!session) {
        return false;
    }
    session.reset();
    return true;
}
