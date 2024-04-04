#include "SyncContext.h"

#include <meojson/json.hpp>

#include "Controller/ControllerAgent.h"
#include "PipelineTask.h"
#include "Task/Actuator.h"
#include "Task/Recognizer.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

SyncContext::SyncContext(InstanceInternalAPI* inst)
    : inst_(inst)
{
}

bool SyncContext::run_task(std::string task, std::string_view param)
{
    LogFunc << VAR(task) << VAR(param);

    if (!inst_) {
        LogError << "Instance is null";
        return false;
    }

    auto json_opt = json::parse(param);
    if (!json_opt) {
        LogError << "Parse param failed" << VAR(param);
        return false;
    }

    PipelineTask pipeline(task, inst_);
    pipeline.set_param(*json_opt);

    return pipeline.run();
}

bool SyncContext::run_recognizer(
    cv::Mat image,
    std::string task,
    std::string_view param,
    cv::Rect& box,
    std::string& detail)
{
    LogFunc << VAR(task) << VAR(param);

    box = cv::Rect();
    detail.clear();

    if (!inst_) {
        LogError << "Instance is null";
        return false;
    }
    if (image.empty()) {
        LogError << "Image is empty";
        return false;
    }

    auto json_opt = json::parse(param);
    if (!json_opt) {
        LogError << "Parse param failed" << VAR(param);
        return false;
    }

    Recognizer recognizer(inst_);

    TaskDataMgr data_mgr(inst_);
    data_mgr.set_param(*json_opt);
    const auto& task_data = data_mgr.get_task_data(task);

    auto reco = recognizer.recognize(image, task_data);

    box = *reco.hit;
    detail = reco.detail.to_string();

    return reco.hit.has_value();
}

bool SyncContext::run_action(
    std::string task,
    std::string_view param,
    cv::Rect cur_box,
    std::string cur_detail)
{
    LogFunc << VAR(task) << VAR(param);

    if (!inst_) {
        LogError << "Instance is null";
        return false;
    }

    auto json_opt = json::parse(param);
    if (!json_opt) {
        LogError << "Parse param failed" << VAR(param);
        return false;
    }

    Actuator actuator(inst_);

    Recognizer::Hit reco_hit = cur_box;

    TaskDataMgr data_mgr(inst_);
    data_mgr.set_param(*json_opt);
    const auto& task_data = data_mgr.get_task_data(task);

    auto ret = actuator.run(reco_hit, json::parse(cur_detail).value_or(cur_detail), task_data);
    return ret;
}

bool SyncContext::click(int x, int y)
{
    LogFunc << VAR(x) << VAR(y);

    auto* ctrl = controller();
    if (!ctrl) {
        LogError << "Controller is null";
        return false;
    }

    auto id = ctrl->post_click(x, y);
    return ctrl->wait(id) == MaaStatus_Success;
}

bool SyncContext::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogFunc << VAR(x1) << VAR(x2) << VAR(y1) << VAR(y2) << VAR(duration);

    auto* ctrl = controller();
    if (!ctrl) {
        LogError << "Controller is null";
        return false;
    }

    auto id = ctrl->post_swipe(x1, x2, y1, y2, duration);
    return ctrl->wait(id) == MaaStatus_Success;
}

bool SyncContext::press_key(int keycode)
{
    LogFunc << VAR(keycode);

    auto* ctrl = controller();
    if (!ctrl) {
        LogError << "Controller is null";
        return false;
    }

    auto id = ctrl->post_press_key(keycode);
    return ctrl->wait(id) == MaaStatus_Success;
}

bool SyncContext::input_text(std::string_view text)
{
    LogFunc << VAR(text);

    auto* ctrl = controller();
    if (!ctrl) {
        LogError << "Controller is null";
        return false;
    }

    auto id = ctrl->post_input_text(text);
    return ctrl->wait(id) == MaaStatus_Success;
}

bool SyncContext::touch_down(int contact, int x, int y, int pressure)
{
    LogFunc << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    auto* ctrl = controller();
    if (!ctrl) {
        LogError << "Controller is null";
        return false;
    }

    auto id = ctrl->post_touch_down(contact, x, y, pressure);
    return ctrl->wait(id) == MaaStatus_Success;
}

bool SyncContext::touch_move(int contact, int x, int y, int pressure)
{
    LogFunc << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    auto* ctrl = controller();
    if (!ctrl) {
        LogError << "Controller is null";
        return false;
    }

    auto id = ctrl->post_touch_move(contact, x, y, pressure);
    return ctrl->wait(id) == MaaStatus_Success;
}

bool SyncContext::touch_up(int contact)
{
    LogFunc << VAR(contact);

    auto* ctrl = controller();
    if (!ctrl) {
        LogError << "Controller is null";
        return false;
    }

    auto id = ctrl->post_touch_up(contact);
    return ctrl->wait(id) == MaaStatus_Success;
}

cv::Mat SyncContext::screencap()
{
    LogFunc;
    auto* ctrl = controller();
    if (!ctrl) {
        LogError << "Controller is null";
        return {};
    }
    auto id = ctrl->post_screencap();
    ctrl->wait(id);

    return ctrl->get_image();
}

MAA_TASK_NS_END