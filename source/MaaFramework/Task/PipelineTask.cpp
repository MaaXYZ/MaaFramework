#include "PipelineTask.h"

#include <sstream>

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Task/Actuator.h"
#include "Task/Recognizer.h"
#include "Utils/ImageIo.h"
#include "Utils/Logger.h"
#include "Utils/Uuid.h"

MAA_TASK_NS_BEGIN

PipelineTask::PipelineTask(std::string entry, InstanceInternalAPI* inst)
    : inst_(inst)
    , entry_(std::move(entry))
    , data_mgr_(inst)
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

    LogInfo << "Task hit:" << hits.task_data.name << VAR(hits.reco_uid) << VAR(hits.reco_hit)
            << VAR(hits.reco_detail);
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
        json::value detail = basic_info()
                             | json::object {
                                   { "list", json::array(list) },
                               };
        notify(MaaMsg_Task_Debug_ListToRecognize, detail);
    }

    bool hit = false;

    Recognizer recognizer(inst_);
    HitResult result;

    for (const std::string& name : list) {
        LogDebug << "recognize:" << name;

        const auto& task_data = data_mgr_.get_task_data(name);
        if (!task_data.enabled) {
            LogDebug << "Task disabled:" << name;
            continue;
        }

        auto reco = recognizer.recognize(image, task_data);

        if (debug_mode()) {
            json::value detail = basic_info()
                                 | json::object {
                                       { "name", name },
                                       { "recognition",
                                         {
                                             { "id", reco.uid },
                                             { "detail", reco.detail },
                                             { "draws_size", reco.draws.size() },
                                         } },
                                       { "hit", reco.hit.has_value() },
                                   };
            notify(MaaMsg_Task_Debug_RecognitionResult, detail);
        }

        if (!reco.hit) {
            continue;
        }

        hit = true;
        result = { .reco_uid = reco.uid,
                   .reco_hit = *std::move(reco.hit),
                   .reco_detail = std::move(reco.detail),
                   .task_data = task_data };
        break;
    }

    if (!hit) {
        return std::nullopt;
    }

    if (debug_mode()) {
        json::value detail = basic_info()
                             | json::object {
                                   { "name", result.task_data.name },
                                   { "recognition",
                                     {
                                         { "id", result.reco_uid },
                                         { "box", result.reco_hit },
                                         { "detail", result.reco_detail },
                                     } },
                                   { "status", "Hit" },
                               };
        notify(MaaMsg_Task_Debug_Hit, detail);
    }

    return result;
}

PipelineTask::RunningResult PipelineTask::run_task(const HitResult& hits)
{
    if (need_to_stop()) {
        LogInfo << "Task interrupted" << VAR(latest_hit_);
        return RunningResult::Interrupted;
    }

    Actuator actuator(inst_);

    const std::string& name = hits.task_data.name;
    uint64_t& run_times = run_times_map_[name];

    json::value detail = basic_info()
                         | json::object { { "id", actuator.uid() },
                                          { "name", name },
                                          { "recognition",
                                            {
                                                { "id", hits.reco_uid },
                                                { "box", hits.reco_hit },
                                                { "detail", hits.reco_detail },
                                            } },
                                          { "run_times", run_times },
                                          { "status", "ReadyToRun" },
                                          { "is_sub", hits.task_data.is_sub } };

    if (hits.task_data.focus) {
        notify(MaaMsg_Task_Focus_ReadyToRun, detail);
    }
    if (debug_mode()) {
        notify(MaaMsg_Task_Debug_ReadyToRun, detail);
    }

    if (hits.task_data.times_limit <= run_times) {
        LogInfo << "Task runout:" << name;

        detail["status"] = "Runout";
        if (hits.task_data.focus) {
            notify(MaaMsg_Task_Focus_Runout, detail);
        }
        if (debug_mode()) {
            notify(MaaMsg_Task_Debug_Runout, detail);
        }

        return RunningResult::Runout;
    }

    auto ret = actuator.run(hits.reco_hit, hits.reco_detail, hits.task_data);

    ++run_times;

    detail["status"] = "Completed";
    detail["run_times"] = run_times;

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
