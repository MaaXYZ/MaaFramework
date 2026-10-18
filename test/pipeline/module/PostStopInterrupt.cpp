#include "PostStopInterrupt.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "MaaFramework/MaaAPI.h"
#include "MaaFramework/MaaMsg.h"

// 回归用例：MaaXYZ/MaaFramework#1564。
// post_stop 后两个阻塞点曾不检查 context_->need_to_stop()：wait_freezes 会跑满 timeout（默认 20s），
// TaskBase::sleep 是纯 sleep_for。表现为 CustomAction 里的 MaaContextRunTask 永不返回，
// 宿主超时强杀后 0xC0000005。
// 本用例用自定义控制器伪造"永不稳定的画面"，在子任务进入 wait_freezes 后由宿主线程 post_stop，
// 断言 MaaContextRunTask 在 1s 内返回。

namespace
{

constexpr int32_t kScreenWidth = 32;
constexpr int32_t kScreenHeight = 32;
constexpr int32_t kScreenType = 16; // CV_8UC3
constexpr long long kStopLatencySlackMs = 1000;

std::mutex g_screencap_mutex;
uint8_t g_frame_counter = 0;

// 每帧递增像素值，wait_freezes 永远等不到"画面稳定"
MaaBool custom_screencap(void* trans_arg, /* out */ MaaImageBuffer* buffer)
{
    std::ignore = trans_arg;

    uint8_t value = 0;
    {
        std::lock_guard lock(g_screencap_mutex);
        g_frame_counter = static_cast<uint8_t>(g_frame_counter + 1);
        value = g_frame_counter;
    }

    std::vector<uint8_t> pixels(kScreenWidth * kScreenHeight * 3, value);
    return MaaImageBufferSetRawData(buffer, pixels.data(), kScreenWidth, kScreenHeight, kScreenType);
}

MaaBool custom_connect(void* trans_arg)
{
    std::ignore = trans_arg;
    return true;
}

MaaBool custom_request_uuid(void* trans_arg, MaaStringBuffer* buffer)
{
    std::ignore = trans_arg;
    return MaaStringBufferSet(buffer, "post-stop-interrupt");
}

MaaControllerFeature custom_get_features(void* trans_arg)
{
    std::ignore = trans_arg;
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
}

MaaBool custom_click(int32_t x, int32_t y, void* trans_arg)
{
    std::ignore = x;
    std::ignore = y;
    std::ignore = trans_arg;
    return true;
}

MaaBool custom_inactive(void* trans_arg)
{
    std::ignore = trans_arg;
    return true;
}

MaaController* create_changing_controller()
{
    static MaaCustomControllerCallbacks callbacks {};
    callbacks.connect = &custom_connect;
    callbacks.request_uuid = &custom_request_uuid;
    callbacks.get_features = &custom_get_features;
    callbacks.screencap = &custom_screencap;
    callbacks.click = &custom_click;
    callbacks.inactive = &custom_inactive;

    return MaaCustomControllerCreate(&callbacks, nullptr);
}

struct SubTaskProbe
{
    std::atomic_bool reached_wait_freezes { false };
    std::atomic_bool returned { false };
    std::atomic<long long> total_ms { -1 };           // 进入 run_task 到返回
    std::atomic<long long> stop_to_return_ms { -1 };  // post_stop 发出到返回，这才是停止延迟
    std::atomic<MaaTaskId> sub_task_id { MaaInvalidId };
    std::chrono::steady_clock::time_point enter_clock;

    // 宿主线程写入、worker 线程读取
    std::atomic<long long> stop_clock_ns { 0 };
};

// 主任务 CustomAction：MaaContextRunTask 跑子任务，子任务卡在 post_wait_freezes
MaaBool main_custom_action(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg)
{
    std::ignore = task_id;
    std::ignore = node_name;
    std::ignore = custom_action_name;
    std::ignore = custom_action_param;
    std::ignore = reco_id;
    std::ignore = box;

    auto* probe = static_cast<SubTaskProbe*>(trans_arg);
    if (!probe) {
        return false;
    }

    // 子任务节点已在资源里定义，这里只做一次空 override
    constexpr std::string_view kChildOverride = R"({"PostStopInterruptChild": {"enabled": true}})";

    probe->enter_clock = std::chrono::steady_clock::now();
    MaaTaskId sub_task_id = MaaContextRunTask(context, "PostStopInterruptChild", std::string(kChildOverride).c_str());

    const auto return_clock = std::chrono::steady_clock::now();
    const auto total = std::chrono::duration_cast<std::chrono::milliseconds>(return_clock - probe->enter_clock);

    probe->total_ms = total.count();

    const long long stop_ns = probe->stop_clock_ns;
    if (stop_ns != 0) {
        const auto stop_clock = std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration(stop_ns));
        probe->stop_to_return_ms = std::chrono::duration_cast<std::chrono::milliseconds>(return_clock - stop_clock).count();
    }

    probe->sub_task_id = sub_task_id;
    probe->returned = true;

    return sub_task_id != MaaInvalidId;
}

// sink 只记录"子任务已进入 wait_freezes"，不在这里调任何宿主 API：
// dispatch 在 worker 线程上无锁遍历 sink 表，回调里再改 sink 表或重入 tasker 都会踩坏它。
void on_context_message(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    std::ignore = handle;
    std::ignore = details_json;

    auto* probe = static_cast<SubTaskProbe*>(trans_arg);
    if (!probe || !message) {
        return;
    }

    if (std::strcmp(message, MaaMsg_Node_WaitFreezes_Starting) == 0) {
        probe->reached_wait_freezes = true;
    }
}

} // namespace

bool post_stop_interrupt(const std::filesystem::path& testset_dir)
{
    auto controller = create_changing_controller();
    auto resource = MaaResourceCreate();

    if (!controller || !resource) {
        std::cout << "PostStopInterrupt: failed to create controller or resource" << std::endl;
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return false;
    }

    // 本用例不涉及推理，固定 CPU 后端，避免在无可用 GPU 的机器上走 DirectML 初始化
    MaaInferenceExecutionProvider ep = MaaInferenceExecutionProvider_CPU;
    MaaResourceSetOption(resource, MaaResOption_InferenceExecutionProvider, &ep, sizeof(ep));

    auto ctrl_id = MaaControllerPostConnection(controller);
    MaaControllerWait(controller, ctrl_id);

    auto bundle_dir = testset_dir / "PostStopInterrupt" / "resource";
    auto res_id = MaaResourcePostBundle(resource, bundle_dir.string().c_str());
    MaaStatus res_status = MaaResourceWait(resource, res_id);

    auto tasker = MaaTaskerCreate();
    MaaTaskerBindResource(tasker, resource);
    MaaTaskerBindController(tasker, controller);

    if (res_status != MaaStatus_Succeeded || !MaaTaskerInited(tasker)) {
        std::cout << "PostStopInterrupt: resource or tasker not inited" << std::endl;
        MaaTaskerDestroy(tasker);
        MaaResourceDestroy(resource);
        MaaControllerDestroy(controller);
        return false;
    }

    SubTaskProbe probe;
    MaaResourceRegisterCustomAction(resource, "PostStopInterruptMainAction", &main_custom_action, &probe);

    auto sink_id = MaaTaskerAddContextSink(tasker, &on_context_message, &probe);

    auto task_id = MaaTaskerPostTask(tasker, "PostStopInterruptMain", "{}");

    // 宿主侧等待子任务进入 wait_freezes 后，由本线程（而非 worker 的回调里）发 post_stop，
    // 与 #1564 的真实调用方一致
    MaaTaskId stop_task_id = MaaInvalidId;
    bool sent_stop = false;
    long long stop_clock_ns = 0;

    {
        constexpr auto kWaitEnter = std::chrono::seconds(10);
        auto deadline = std::chrono::steady_clock::now() + kWaitEnter;
        while (std::chrono::steady_clock::now() < deadline && !probe.reached_wait_freezes) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        if (probe.reached_wait_freezes) {
            stop_clock_ns = std::chrono::steady_clock::now().time_since_epoch().count();
            probe.stop_clock_ns = stop_clock_ns;
            stop_task_id = MaaTaskerPostStop(tasker);
            sent_stop = true;
        }
    }

    // 兜底 10s：修复前这里必然等满（或永不返回）
    {
        constexpr auto kOuterTimeout = std::chrono::seconds(10);
        auto deadline = std::chrono::steady_clock::now() + kOuterTimeout;
        while (std::chrono::steady_clock::now() < deadline && !probe.returned) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    bool sub_returned = probe.returned;
    long long total_ms = probe.total_ms;
    long long stop_to_return_ms = probe.stop_to_return_ms;

    bool passed = false;
    if (!sent_stop) {
        std::cout << "PostStopInterrupt: sub task never reached wait_freezes" << std::endl;
    }
    else if (!sub_returned) {
        std::cout << "PostStopInterrupt: MaaContextRunTask did not return after post_stop" << std::endl;
    }
    else if (stop_to_return_ms < 0) {
        std::cout << "PostStopInterrupt: post_stop timestamp missing" << std::endl;
    }
    else if (stop_to_return_ms > kStopLatencySlackMs) {
        // 只计 post_stop 发出到 run_task 返回，不含子任务启动耗时
        std::cout << std::format(
                         "PostStopInterrupt: MaaContextRunTask returned {}ms after post_stop (total {}ms), expected <= {}ms",
                         stop_to_return_ms,
                         total_ms,
                         kStopLatencySlackMs)
                  << std::endl;
    }
    else {
        std::cout << std::format(
                         "PostStopInterrupt: MaaContextRunTask returned {}ms after post_stop (total {}ms)",
                         stop_to_return_ms,
                         total_ms)
                  << std::endl;
        passed = true;
    }

    // 断言结论先落盘再清理句柄，避免清理阶段的环境问题掩盖用例结果
    std::cout.flush();

    // 等所有任务退出后再动 sink 表：dispatch 在 worker 线程上无锁遍历 sink 表，
    // 并发调用 MaaTaskerRemoveContextSink 会踩坏 std::map。
    MaaTaskerWait(tasker, task_id);
    if (stop_task_id != MaaInvalidId) {
        MaaTaskerWait(tasker, stop_task_id);
    }

    MaaTaskerRemoveContextSink(tasker, sink_id);

    MaaTaskerDestroy(tasker);
    MaaResourceDestroy(resource);
    MaaControllerDestroy(controller);

    return passed;
}
