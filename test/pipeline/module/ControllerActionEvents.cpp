#include "ControllerActionEvents.h"

#include <meojson/json.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <format>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <vector>

#include "MaaFramework/MaaAPI.h"
#include "MaaFramework/MaaMsg.h"

// 回归用例：MaaXYZ/MaaFramework#1608。
// 公共 post_* 曾先把动作入队、再另行登记“这次要派发事件”。worker 在两步之间取走动作时，
// Starting / Succeeded 一条都不发，调用方也收不到任何报错。
// 每次都等上一次动作完成再 post，使 worker 总是空闲地等在队列上、入队即被唤醒，竞争窗口最大；
// 批量 post 后再等反而会让 worker 落后于调用方、几乎复现不出来。
// 多个控制器并发压测以提高命中率：修复前在 12 核 Windows 上连续 30 轮全部失败（每轮漏发 2~164 条，单轮约 3s）。

namespace
{

constexpr int kControllerCount = 8;
constexpr int kClicksPerController = 500;

struct ControllerProbe
{
    // 由控制器回调计数，与事件无关，用来确认动作确实执行过
    std::atomic<int64_t> clicks { 0 };

    std::mutex mutex;
    int64_t starting = 0;
    int64_t succeeded = 0;
    int64_t failed = 0;
    std::vector<std::string> starting_actions;
};

void on_controller_message(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    std::ignore = handle;

    auto* probe = static_cast<ControllerProbe*>(trans_arg);
    if (!probe || !message || !details_json) {
        return;
    }

    std::lock_guard lock(probe->mutex);
    if (std::strcmp(message, MaaMsg_Controller_Action_Starting) == 0) {
        ++probe->starting;
        auto details = json::parse(details_json);
        probe->starting_actions.emplace_back(details ? details->get("action", std::string()) : std::string());
    }
    else if (std::strcmp(message, MaaMsg_Controller_Action_Succeeded) == 0) {
        ++probe->succeeded;
    }
    else if (std::strcmp(message, MaaMsg_Controller_Action_Failed) == 0) {
        ++probe->failed;
    }
}

MaaBool custom_connect(void* trans_arg)
{
    std::ignore = trans_arg;
    return true;
}

MaaBool custom_request_uuid(void* trans_arg, MaaStringBuffer* buffer)
{
    std::ignore = trans_arg;
    return MaaStringBufferSet(buffer, "controller-action-events");
}

MaaControllerFeature custom_get_features(void* trans_arg)
{
    std::ignore = trans_arg;
    return MaaControllerFeature_None;
}

MaaBool custom_screencap(void* trans_arg, /* out */ MaaImageBuffer* buffer)
{
    std::ignore = trans_arg;

    constexpr int32_t kSize = 32;
    constexpr int32_t kType = 16; // CV_8UC3
    std::vector<uint8_t> pixels(kSize * kSize * 3, 0);
    return MaaImageBufferSetRawData(buffer, pixels.data(), kSize, kSize, kType);
}

MaaBool custom_click(int32_t x, int32_t y, void* trans_arg)
{
    std::ignore = x;
    std::ignore = y;

    if (auto* probe = static_cast<ControllerProbe*>(trans_arg)) {
        ++probe->clicks;
    }
    return true;
}

MaaBool custom_inactive(void* trans_arg)
{
    std::ignore = trans_arg;
    return true;
}

MaaCustomControllerCallbacks make_callbacks()
{
    MaaCustomControllerCallbacks callbacks { };
    callbacks.connect = &custom_connect;
    callbacks.request_uuid = &custom_request_uuid;
    callbacks.get_features = &custom_get_features;
    callbacks.screencap = &custom_screencap;
    callbacks.click = &custom_click;
    callbacks.inactive = &custom_inactive;
    return callbacks;
}

MaaController* create_controller(ControllerProbe& probe)
{
    // 框架只保存回调表的指针，回调表必须比控制器活得久
    static MaaCustomControllerCallbacks callbacks = make_callbacks();

    auto* controller = MaaCustomControllerCreate(&callbacks, &probe);
    if (!controller) {
        return nullptr;
    }

    // 点击坐标缩放依赖首帧分辨率，先截一次图；之后才挂 sink，压测期间就只会有 click 事件
    if (MaaControllerWait(controller, MaaControllerPostConnection(controller)) != MaaStatus_Succeeded
        || MaaControllerWait(controller, MaaControllerPostScreencap(controller)) != MaaStatus_Succeeded) {
        MaaControllerDestroy(controller);
        return nullptr;
    }
    return controller;
}

void click_repeatedly(MaaController* controller, int64_t& succeeded_posts)
{
    for (int i = 0; i < kClicksPerController; ++i) {
        if (MaaControllerWait(controller, MaaControllerPostClick(controller, 1, 1)) == MaaStatus_Succeeded) {
            ++succeeded_posts;
        }
    }
}

bool run_concurrent_clicks(
    const std::array<MaaController*, kControllerCount>& controllers,
    std::array<ControllerProbe, kControllerCount>& probes)
{
    for (int i = 0; i < kControllerCount; ++i) {
        if (!controllers[i] || MaaControllerAddSink(controllers[i], &on_controller_message, &probes[i]) == MaaInvalidId) {
            std::cout << "ControllerActionEvents: failed to create controller or add sink" << std::endl;
            return false;
        }
    }

    std::array<int64_t, kControllerCount> succeeded_posts { };
    std::vector<std::thread> threads;
    threads.reserve(kControllerCount);
    for (int i = 0; i < kControllerCount; ++i) {
        threads.emplace_back(click_repeatedly, controllers[i], std::ref(succeeded_posts[i]));
    }
    for (auto& thread : threads) {
        thread.join();
    }

    // 事件在 worker 上同步派发，且早于动作的完成标记；所有 wait 都返回后计数已经定形
    int64_t posts = 0;
    int64_t succeeded = 0;
    int64_t starting = 0;
    int64_t notified = 0;
    int64_t failed = 0;
    for (int i = 0; i < kControllerCount; ++i) {
        std::lock_guard lock(probes[i].mutex);
        posts += kClicksPerController;
        succeeded += succeeded_posts[i];
        starting += probes[i].starting;
        notified += probes[i].succeeded;
        failed += probes[i].failed;
    }

    if (succeeded != posts || starting != posts || notified != posts || failed != 0) {
        std::cout << std::format(
            "ControllerActionEvents: {} posted clicks ({} succeeded) got {} Starting / {} Succeeded / {} Failed events",
            posts,
            succeeded,
            starting,
            notified,
            failed)
                  << std::endl;
        return false;
    }

    return true;
}

bool check_public_post_always_notifies()
{
    // 控制器回调与 sink 都指向探针，探针必须比控制器活得久
    std::array<ControllerProbe, kControllerCount> probes;
    std::array<MaaController*, kControllerCount> controllers { };
    for (int i = 0; i < kControllerCount; ++i) {
        controllers[i] = create_controller(probes[i]);
    }

    bool passed = run_concurrent_clicks(controllers, probes);

    for (auto* controller : controllers) {
        if (controller) {
            MaaControllerDestroy(controller);
        }
    }
    return passed;
}

bool run_internal_click(MaaTasker* tasker, MaaResource* resource, MaaController* controller, ControllerProbe& probe)
{
    if (!tasker || !resource || !controller || !MaaTaskerBindResource(tasker, resource) || !MaaTaskerBindController(tasker, controller)
        || !MaaTaskerInited(tasker)) {
        std::cout << "ControllerActionEvents: failed to init tasker" << std::endl;
        return false;
    }

    if (MaaControllerAddSink(controller, &on_controller_message, &probe) == MaaInvalidId) {
        std::cout << "ControllerActionEvents: failed to add sink" << std::endl;
        return false;
    }

    constexpr std::string_view kOverride =
        R"({"InternalClick": {"action": "Click", "target": [1, 1, 1, 1], "pre_delay": 0, "post_delay": 0}})";
    auto task_id = MaaTaskerPostTask(tasker, "InternalClick", std::string(kOverride).c_str());
    if (MaaTaskerWait(tasker, task_id) != MaaStatus_Succeeded) {
        std::cout << "ControllerActionEvents: internal click task failed" << std::endl;
        return false;
    }

    // 任务结束时 tasker 会经公共接口 post_inactive 一次，只有它应当派发事件。
    // 同时核对控制器确实收到了节点里的点击，避免节点没执行时用例空过
    std::lock_guard lock(probe.mutex);
    const bool only_inactive = probe.starting_actions == std::vector<std::string> { "inactive" };
    if (probe.clicks != 1 || !only_inactive || probe.succeeded != 1 || probe.failed != 0) {
        std::string actions;
        for (const auto& action : probe.starting_actions) {
            actions += action + " ";
        }
        std::cout << std::format(
            "ControllerActionEvents: internal click should only notify the trailing inactive, got {} clicks, Starting for [ {}], "
            "{} Succeeded, {} Failed",
            probe.clicks.load(),
            actions,
            probe.succeeded,
            probe.failed)
                  << std::endl;
        return false;
    }

    return true;
}

// Tasker 内部发起的动作（节点里的截图、点击）不派发 Starting / Succeeded，
// 否则接入方每个节点都会收到大量并非自己发起的控制器事件
bool check_internal_actions_stay_silent()
{
    ControllerProbe probe;
    auto* controller = create_controller(probe);
    auto* resource = MaaResourceCreate();
    auto* tasker = MaaTaskerCreate();

    bool passed = run_internal_click(tasker, resource, controller, probe);

    MaaTaskerDestroy(tasker);
    MaaResourceDestroy(resource);
    if (controller) {
        MaaControllerDestroy(controller);
    }
    return passed;
}

} // namespace

bool controller_action_events()
{
    if (!check_public_post_always_notifies() || !check_internal_actions_stay_silent()) {
        return false;
    }

    std::cout << "ControllerActionEvents: passed" << std::endl;
    return true;
}
