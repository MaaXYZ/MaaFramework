#include "PipelineTask.h"

#include <sstream>

#include "Controller/ControllerAgent.h"
#include "Instance/InstanceStatus.h"
#include "MaaFramework/MaaMsg.h"
#include "Option/GlobalOptionMgr.h"
#include "Resource/ResourceMgr.h"
#include "Task/CustomAction.h"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"
#include "Utils/Uuid.h"

MAA_TASK_NS_BEGIN

PipelineTask::PipelineTask(std::string entry, InstanceInternalAPI* inst)
    : inst_(inst)
    , entry_(std::move(entry))
    , data_mgr_(inst)
    , recognizer_(inst)
    , actuator_(inst)
{
}

bool PipelineTask::run()
{
    LogFunc << VAR(entry_);

    std::vector<std::string> next_list = { entry_ };
    std::stack<std::string> breakpoints_stack;
    std::string pre_breakpoint;

    RunningResult ret = RunningResult::Success;

    TaskData new_hits = data_mgr_.get_task_data(entry_);
    while (!next_list.empty() && !need_to_stop()) {
        auto timeout = new_hits.timeout;
        ret = find_first_and_run(next_list, timeout, new_hits);

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

PipelineTask::RunningResult PipelineTask::find_first_and_run(
    const std::vector<std::string>& list,
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

        if (need_to_stop()) {
            LogInfo << "Task interrupted" << VAR(latest_hit_);
            return RunningResult::Interrupted;
        }

        if (std::chrono::steady_clock::now() - start_time > timeout) {
            LogInfo << "Task timeout" << VAR(latest_hit_) << VAR(timeout);
            return RunningResult::Timeout;
        }
    }

    LogInfo << "Task hit:" << hits.task_data.name << hits.rec_result.box;
    latest_hit_ = hits.task_data.name;

    auto run_ret = run_task(hits);

    found_data = std::move(hits.task_data);

    return run_ret;
}

std::optional<PipelineTask::HitResult>
    PipelineTask::find_first(const std::vector<std::string>& list)
{
    if (!controller()) {
        LogError << "Controller not binded";
        return std::nullopt;
    }
    if (need_to_stop()) {
        LogInfo << "Task interrupted" << VAR(latest_hit_);
        return std::nullopt;
    }

    LogFunc << VAR(latest_hit_) << VAR(list);

    cv::Mat image = controller()->screencap();

    if (image.empty()) {
        LogError << "Image is empty";
        return std::nullopt;
    }

    if (need_to_stop()) {
        LogInfo << "Task interrupted" << VAR(latest_hit_);
        return std::nullopt;
    }

    if (debug_mode()) {
        auto screencap_path = dump_image(image);
        json::value detail = basic_info()
                             | json::object {
                                   { "list", json::array(list) },
                                   { "screencap", path_to_utf8_string(screencap_path) },
                               };
        notify(MaaMsg_Task_Debug_ListToRecognize, detail);
    }

    bool hit = false;
    HitResult result;

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

        hit = true;
        result = { .rec_result = *std::move(rec_opt), .task_data = task_data };
        break;
    }

    if (!hit) {
        return std::nullopt;
    }

    if (debug_mode()) {
        json::value detail = basic_info()
                             | json::object {
                                   { "name", result.task_data.name },
                                   { "recognition", result.rec_result.detail },
                                   { "status", "Hit" },
                               };
        notify(MaaMsg_Task_Debug_Hit, detail);
    }

    return result;
}

PipelineTask::RunningResult PipelineTask::run_task(const HitResult& hits)
{
    if (!status()) {
        LogError << "Status not binded";
        return RunningResult::InternalError;
    }

    if (need_to_stop()) {
        LogInfo << "Task interrupted" << VAR(latest_hit_);
        return RunningResult::Interrupted;
    }

    const std::string& name = hits.task_data.name;
    uint64_t run_times = status()->get_run_times(name);

    json::value detail = basic_info()
                         | json::object { { "name", name },
                                          { "recognition", hits.rec_result.detail },
                                          { "run_times", run_times },
                                          { "status", "ReadyToRun" },
                                          { "is_sub", hits.task_data.is_sub } };

    status()->set_task_result(name, detail);
    if (hits.task_data.focus) {
        notify(MaaMsg_Task_Focus_ReadyToRun, detail);
    }
    if (debug_mode()) {
        notify(MaaMsg_Task_Debug_ReadyToRun, detail);
    }

    if (hits.task_data.times_limit <= run_times) {
        LogInfo << "Task runout:" << name;

        detail["status"] = "Runout";
        status()->set_task_result(name, detail);
        if (hits.task_data.focus) {
            notify(MaaMsg_Task_Focus_Runout, detail);
        }
        if (debug_mode()) {
            notify(MaaMsg_Task_Debug_Runout, detail);
        }

        return RunningResult::Runout;
    }

    auto ret = actuator_.run(hits.rec_result, hits.task_data);
    status()->increase_run_times(name);

    detail["status"] = "Completed";
    detail["run_times"] = run_times + 1;
    status()->set_task_result(name, detail);
    if (hits.task_data.focus) {
        notify(MaaMsg_Task_Focus_Completed, detail);
    }
    if (debug_mode()) {
        notify(MaaMsg_Task_Debug_Completed, detail);
    }

    return ret ? RunningResult::Success : RunningResult::InternalError;
}

bool PipelineTask::debug_mode() const
{
    return GlobalOptionMgr::get_instance().debug_message();
}

json::object PipelineTask::basic_info()
{
    return {
        { "id", task_id_ },
        { "entry", entry() },
        { "hash", resource() ? resource()->get_hash() : std::string() },
        { "uuid", controller() ? controller()->get_uuid() : std::string() },
        { "latest_hit", latest_hit_ },
    };
}

std::filesystem::path PipelineTask::dump_image(const cv::Mat& image) const
{
    if (image.empty()) {
        return {};
    }

    const auto& log_dir = GlobalOptionMgr::get_instance().log_dir();
    if (log_dir.empty()) {
        return {};
    }

    std::string filename = std::format("{}-{}.png", format_now_for_filename(), make_uuid());
    auto filepath = log_dir / "screencap" / path(filename);
    bool ret = MAA_NS::imwrite(filepath, image);
    if (!ret) {
        return {};
    }

    return filepath;
}

MAA_TASK_NS_END
