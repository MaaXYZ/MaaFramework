#include "SyncContext.h"

#include <meojson/json.hpp>

#include "Controller/ControllerMgr.h"
#include "Instance/InstanceStatus.h"
#include "PipelineTask.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

SyncContext::SyncContext(InstanceInternalAPI* inst) : inst_(inst) {}

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
