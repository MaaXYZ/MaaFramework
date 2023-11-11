#include "PipelineTask.h"

#include <sstream>

#include "Controller/ControllerAgent.h"
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

    std::vector<std::string> next_list = { entry_ };
    std::stack<std::string> breakpoints_stack;
    std::string pre_breakpoint;

    RunningResult ret = RunningResult::Success;

    TaskData new_hits = data_mgr_.get_task_data(entry_);
    while (!next_list.empty() && !need_exit()) {
        auto timeout = new_hits.timeout;
        ret = find_first_and_run(next_list, timeout, new_hits);

        cur_task_name_ = new_hits.name;

        switch (ret) {
        case RunningResult::Success:
            next_list = new_hits.next;
            break;
        case RunningResult::Timeout:
            next_list = new_hits.timeout_next;
            break;
        case RunningResult::Runout:
            next_list = new_hits.runout_next;
            break;
        case RunningResult::Interrupted:
            LogInfo << "Task interrupted:" << new_hits.name;
            return true;
        case RunningResult::InternalError:
            LogError << "Task InternalError:" << new_hits.name;
            return false;
        default:
            break;
        }

        if (new_hits.is_sub) {
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
            pre_breakpoint = new_hits.name;
        }
    }

    return ret == RunningResult::Success;
}

bool PipelineTask::set_param(const json::value& param)
{
    return data_mgr_.set_param(param);
}

PipelineTask::RunningResult PipelineTask::find_first_and_run(const std::vector<std::string>& list,
                                                             std::chrono::milliseconds timeout,
                                                             /*out*/ MAA_RES_NS::TaskData& found_data)
{
    HitResult hits;

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        auto find_opt = find_first(list);
        if (find_opt) {
            hits = *std::move(find_opt);
            break;
        }

        if (std::chrono::steady_clock::now() - start_time > timeout) {
            LogInfo << "Task timeout" << VAR(cur_task_name_) << VAR(timeout);
            return RunningResult::Timeout;
        }
        if (need_exit()) {
            LogInfo << "Task interrupted" << VAR(cur_task_name_);
            return RunningResult::Interrupted;
        }
    }
    LogInfo << "Task hit:" << hits.task_data.name << hits.rec_result.box;

    auto run_ret = run_task(hits);

    found_data = std::move(hits.task_data);

    return run_ret;
}

std::optional<PipelineTask::HitResult> PipelineTask::find_first(const std::vector<std::string>& list)
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
        return HitResult { .rec_result = *std::move(rec_opt), .task_data = task_data };
    }
    return std::nullopt;
}

PipelineTask::RunningResult PipelineTask::run_task(const HitResult& hits)
{
    if (!status()) {
        LogError << "Status not binded";
        return RunningResult::InternalError;
    }

    if (need_exit()) {
        LogInfo << "Task interrupted" << VAR(cur_task_name_);
        return RunningResult::Interrupted;
    }

    const std::string& name = hits.task_data.name;
    uint64_t run_times = status()->get_run_times(name);

    json::value detail = {
        { "id", task_id_ },
        { "entry", entry() },
        { "name", name },
        { "hash", resource() ? resource()->get_hash() : std::string() },
        { "uuid", controller() ? controller()->get_uuid() : std::string() },
        { "recognition", hits.rec_result.detail },
        { "run_times", run_times },
        { "last_time", format_now() },
        { "status", "Hit" },
    };

    status()->set_task_result(name, detail);
    if (hits.task_data.focus) {
        notify(MaaMsg_Task_Focus_Hit, detail);
    }

    if (hits.task_data.times_limit <= run_times) {
        LogInfo << "Task runout:" << name;

        detail["status"] = "Runout";
        status()->set_task_result(name, detail);
        if (hits.task_data.focus) {
            notify(MaaMsg_Task_Focus_Runout, detail);
        }

        return RunningResult::Runout;
    }

    auto ret = actuator_.run(hits.rec_result, hits.task_data);
    status()->increase_run_times(name);

    detail["status"] = "Completed";
    detail["last_time"] = format_now();
    detail["run_times"] = run_times + 1;
    status()->set_task_result(name, detail);
    if (hits.task_data.focus) {
        notify(MaaMsg_Task_Focus_Completed, detail);
    }

    return ret ? RunningResult::Success : RunningResult::InternalError;
}

MAA_TASK_NS_END
