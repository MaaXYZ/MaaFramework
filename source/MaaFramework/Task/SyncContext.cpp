#include "SyncContext.h"

#include <meojson/json.hpp>

#include "Controller/ControllerMgr.h"
#include "Instance/InstanceStatus.h"
#include "PipelineTask.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

SyncContext::SyncContext(TaskInstAPI& task_inst) : task_inst_(task_inst), inst_(task_inst_.inst()) {}

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

    PipelineTask pipeline(task, task_inst_);
    pipeline.set_param(*json_opt);

    return pipeline.run();
}

bool SyncContext::run_recognizer(cv::Mat image, std::string task, std::string_view param, cv::Rect& box,
                                 std::string& detail)
{
    LogFunc << VAR(task) << VAR(param);

    box = cv::Rect();
    detail.clear();

    if (!inst_) {
        LogError << "Instance is null";
        return false;
    }

    auto json_opt = json::parse(param);
    if (!json_opt) {
        LogError << "Parse param failed" << VAR(param);
        return false;
    }

    PipelineTask pipeline(task, task_inst_);
    pipeline.set_param(*json_opt);

    const auto& task_data = pipeline.data_mgr().get_task_data(task);
    auto opt = pipeline.recognizer().recognize(image, task_data);
    if (!opt) {
        return false;
    }

    box = opt->box;
    detail = opt->detail.to_string();
    return true;
}

bool SyncContext::run_action(std::string task, std::string_view param, cv::Rect cur_box, std::string cur_detail)
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

    PipelineTask pipeline(task, task_inst_);
    pipeline.set_param(*json_opt);

    Recognizer::Result rec_result { .box = cur_box, .detail = std::move(cur_detail) };
    const auto& task_data = pipeline.data_mgr().get_task_data(task);
    auto ret = pipeline.actuator().run(rec_result, task_data);
    return ret;
}

void SyncContext::click(int x, int y)
{
    LogFunc << VAR(x) << VAR(y);

    auto* ctrl = controller();
    if (!ctrl) {
        LogError << "Controller is null";
        return;
    }

    auto id = ctrl->post_click(x, y);
    ctrl->wait(id);
}

void SyncContext::swipe(std::vector<int> x_steps, std::vector<int> y_steps, std::vector<int> step_delay)
{
    LogFunc << VAR(x_steps) << VAR(y_steps) << VAR(step_delay);

    auto* ctrl = controller();
    if (!ctrl) {
        LogError << "Controller is null";
        return;
    }

    auto id = ctrl->post_swipe(std::move(x_steps), std::move(y_steps), std::move(step_delay));
    ctrl->wait(id);
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

std::string SyncContext::task_result(const std::string& task_name) const
{
    if (!status()) {
        LogError << "Instance status is null";
        return {};
    }

    return status()->get_pipeline_task_result(task_name).to_string();
}

MAA_TASK_NS_END
