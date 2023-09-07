#include "PipelineTask.h"

#include <sstream>

#include "Controller/ControllerMgr.h"
#include "Instance/InstanceStatus.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Task/CustomAction.h"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

PipelineTask::PipelineTask(std::string entry, InstanceInternalAPI* inst)
    : inst_(inst), entry_(std::move(entry)), data_mgr_(inst), recognizer_(inst), actuator_(inst)
{}

bool PipelineTask::run()
{
    LogFunc << VAR(entry_);

    if (!resource()) {
        LogError << "Resource not binded";
        return false;
    }

    auto cur_task = data_mgr_.get_task_data(entry_);
    cur_task_name_ = cur_task.name;
    std::vector<std::string> next_list = { entry_ };
    std::stack<std::string> breakpoints_stack;
    std::string pre_breakpoint;

    RunningResult ret = RunningResult::Success;
    while (!next_list.empty() && !need_exit()) {
        ret = find_first_and_run(next_list, cur_task.timeout, cur_task);
        cur_task_name_ = cur_task.name;

        switch (ret) {
        case RunningResult::Success:
            next_list = cur_task.next;
            break;
        case RunningResult::Timeout:
            next_list = cur_task.timeout_next;
            break;
        case RunningResult::Runout:
            next_list = cur_task.runout_next;
            break;
        case RunningResult::Interrupted:
            return true;
        default:
            break;
        }

        if (cur_task.is_sub) {
            breakpoints_stack.emplace(pre_breakpoint);
            LogInfo << "breakpoints add" << pre_breakpoint;
        }

        if (next_list.empty() && !breakpoints_stack.empty()) {
            std::string top_bp = std::move(breakpoints_stack.top());
            breakpoints_stack.pop();
            pre_breakpoint = top_bp;
            next_list = data_mgr_.get_task_data(top_bp).next;
            LogInfo << "breakpoints pop" << VAR(top_bp) << VAR(next_list);
        }
        else {
            pre_breakpoint = cur_task.name;
        }
    }

    return ret == RunningResult::Success;
}

bool PipelineTask::set_param(const json::value& param)
{
    return data_mgr_.set_param(param);
}

PipelineTask::RunningResult PipelineTask::find_first_and_run(const std::vector<std::string>& list,
                                                             std::chrono::milliseconds find_timeout,
                                                             /*out*/ MAA_RES_NS::TaskData& found_data)
{
    if (!status()) {
        LogError << "Status not binded";
        return RunningResult::InternalError;
    }
    RecognitionResult result;

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        auto find_opt = find_first(list);
        if (find_opt) {
            result = *std::move(find_opt);
            break;
        }

        if (std::chrono::steady_clock::now() - start_time > find_timeout) {
            return RunningResult::Timeout;
        }
        if (need_exit()) {
            return RunningResult::Interrupted;
        }
    }
    if (need_exit()) {
        return RunningResult::Interrupted;
    }
    const std::string& name = result.task_data.name;
    LogInfo << "Task hit:" << name << VAR(result.rec_result.box);

    uint64_t run_times = status()->get_pipeline_run_times(name);

    json::value detail = {
        { "id", task_id_ },
        { "entry", entry() },
        { "name", name },
        { "hash", resource() ? resource()->get_hash() : std::string() },
        { "uuid", controller() ? controller()->get_uuid() : std::string() },
        { "recognition", result.rec_result.detail },
        { "run_times", run_times },
        { "last_time", format_now() },
        { "status", "Hit" },
    };

    status()->set_pipeline_task_result(name, detail);
    if (result.task_data.focus) {
        notify(MaaMsg_Task_Focus_Hit, detail);
    }

    if (result.task_data.times_limit <= run_times) {
        LogInfo << "Task runout:" << name;

        detail["status"] = "Runout";
        status()->set_pipeline_task_result(name, detail);
        if (result.task_data.focus) {
            notify(MaaMsg_Task_Focus_Runout, detail);
        }

        found_data = std::move(result.task_data);
        return RunningResult::Runout;
    }

    auto ret = actuator_.run(result.rec_result, result.task_data);
    status()->increase_pipeline_run_times(name);

    detail["status"] = "Completed";
    detail["last_time"] = format_now();
    detail["run_times"] = run_times + 1;
    status()->set_pipeline_task_result(name, detail);
    if (result.task_data.focus) {
        notify(MaaMsg_Task_Focus_Completed, detail);
    }

    found_data = std::move(result.task_data);
    return ret ? RunningResult::Success : RunningResult::Interrupted;
}

std::optional<PipelineTask::RecognitionResult> PipelineTask::find_first(const std::vector<std::string>& list)
{
    if (!controller()) {
        LogError << "Controller not binded";
        return std::nullopt;
    }

    LogFunc << VAR(cur_task_name_) << VAR(list);

    cv::Mat image = controller()->screencap();

    for (const std::string& name : list) {
        LogDebug << "recognize:" << name;
        const auto& task_data = data_mgr_.get_task_data(name);
        if (!task_data.enabled) {
            LogDebug << "Task disabled:" << name;
            continue;
        }

        auto rec_opt = recognizer_.recognize(image, task_data);
        if (!rec_opt) {
            continue;
        }
        return RecognitionResult { .rec_result = *std::move(rec_opt), .task_data = task_data };
    }
    return std::nullopt;
}

MAA_TASK_NS_END
