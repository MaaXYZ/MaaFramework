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
// 本用例用自定义控制器伪造"永不稳定的画面"，在子任务进入 wait_freezes 后 post_stop，
// 断言 MaaContextRunTask 在 1s 内返回。

namespace
{

constexpr int32_t kScreenWidth = 32;
constexpr int32_t kScreenHeight = 32;
constexpr int32_t kScreenType = 16; // CV_8UC3
constexpr long long kReturnSlackMs = 1000;

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
    std::atomic_bool returned { false };
    std::atomic<long long> elapsed_ms { -1 };
    std::atomic<MaaTaskId> sub_task_id { MaaInvalidId };
    std::chrono::steady_clock::time_point enter_clock;
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
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - probe->enter_clock);

    probe->elapsed_ms = elapsed.count();
    probe->sub_task_id = sub_task_id;
    probe->returned = true;

    return sub_task_id != MaaInvalidId;
}

struct StopTrigger
{
    MaaTasker* tasker = nullptr;
    std::atomic_bool fired { false };
};

// 等子任务真正进入 wait_freezes（Node.WaitFreezes.Starting）再 post_stop，
// 避免 post_stop 撞上子任务启动瞬间而被 stop 语义直接吞掉
void on_context_message(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    std::ignore = handle;
    std::ignore = details_json;

    auto* trigger = static_cast<StopTrigger*>(trans_arg);
    if (!trigger || !message || !trigger->tasker) {
        return;
    }

    if (std::strcmp(message, MaaMsg_Node_WaitFreezes_Starting) != 0) {
        return;
    }

    bool expected = false;
    if (!trigger->fired.compare_exchange_strong(expected, true)) {
        return;
    }

    MaaTaskerPostStop(trigger->tasker);
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

    StopTrigger trigger { .tasker = tasker };
    auto sink_id = MaaTaskerAddContextSink(tasker, &on_context_message, &trigger);

    auto task_id = MaaTaskerPostTask(tasker, "PostStopInterruptMain", "{}");

    // 兜底 10s：修复前这里必然等满（或永不返回）
    constexpr auto kOuterTimeout = std::chrono::seconds(10);
    auto deadline = std::chrono::steady_clock::now() + kOuterTimeout;
    while (std::chrono::steady_clock::now() < deadline && !probe.returned) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    MaaTaskerRemoveContextSink(tasker, sink_id);

    bool stopped_fired = trigger.fired;
    bool sub_returned = probe.returned;
    long long sub_elapsed_ms = probe.elapsed_ms;

    bool passed = false;
    if (!stopped_fired) {
        std::cout << "PostStopInterrupt: sub task never reached wait_freezes" << std::endl;
    }
    else if (!sub_returned) {
        std::cout << "PostStopInterrupt: MaaContextRunTask did not return after post_stop" << std::endl;
    }
    else if (sub_elapsed_ms < 0 || sub_elapsed_ms > kReturnSlackMs) {
        std::cout << std::format("PostStopInterrupt: MaaContextRunTask returned in {}ms, expected <= {}ms", sub_elapsed_ms, kReturnSlackMs)
                  << std::endl;
    }
    else {
        std::cout << std::format("PostStopInterrupt: MaaContextRunTask returned in {}ms after post_stop", sub_elapsed_ms) << std::endl;
        passed = true;
    }

    // 断言结论先落盘再清理句柄，避免清理阶段的环境问题掩盖用例结果
    std::cout.flush();

    MaaTaskerWait(tasker, task_id);
    MaaTaskerDestroy(tasker);

    MaaResourceDestroy(resource);
    MaaControllerDestroy(controller);

    return passed;
}
