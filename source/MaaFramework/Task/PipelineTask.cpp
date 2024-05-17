#include "PipelineTask.h"

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "Global/UniqueResultBank.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Utils/JsonExt.hpp"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

PipelineTask::PipelineTask(std::string entry, InstanceInternalAPI* inst)
    : inst_(inst)
    , entry_(std::move(entry))
    , data_mgr_(inst)
{
}

bool PipelineTask::run()
{
    switch (run_type_) {
    case RunType::Pipeline:
        return run_pipeline();
    case RunType::Recognition:
        return run_recognition_only();
    case RunType::Action:
        return run_action_only();
    default:
        LogError << "Unknown run type";
        return false;
    }
}

bool PipelineTask::run_pipeline()
{
    LogFunc << VAR(entry_);

    std::vector<std::string> next_list = { entry_ };
    std::stack<std::string> breakpoints_stack;
    std::string pre_breakpoint;

    NodeStatus ret = NodeStatus::Success;

    TaskData new_hits = data_mgr_.get_task_data(entry_);
    while (!next_list.empty() && !need_to_stop()) {
        pre_hit_task_ = new_hits.name;
        ret = find_first_and_run(next_list, new_hits.timeout, new_hits);

        switch (ret) {
        case NodeStatus::Success:
            next_list = new_hits.next;
            break;
        case NodeStatus::Timeout:
            next_list = new_hits.timeout_next;
            break;
        case NodeStatus::Runout:
            next_list = new_hits.runout_next;
            break;
        case NodeStatus::Interrupted:
            LogInfo << "Task interrupted:" << new_hits.name;
            return true;
        case NodeStatus::InternalError:
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

    return ret == NodeStatus::Success;
}

bool PipelineTask::run_recognition_only()
{
    LogFunc << VAR(entry_);

    auto hit_opt = find_first({ entry_ });
    if (hit_opt) {
        NodeDetail node { .hits = std::move(*hit_opt), .status = NodeStatus::Runout };
        auto nid = node.node_id;
        add_node_detail(nid, std::move(node));
    }

    return hit_opt.has_value();
}

bool PipelineTask::run_action_only()
{
    LogFunc << VAR(entry_);

    HitDetail fake_hit { .task_data = data_mgr_.get_task_data(entry_) };
    return run_task(fake_hit) == NodeStatus::Success;
}

bool PipelineTask::set_param(const json::value& param)
{
    return data_mgr_.set_param(param);
}

bool PipelineTask::query_node_detail(
    MaaNodeId node_id,
    std::string& name,
    MaaRecoId& reco_id,
    bool& completed)
{
    const auto& bank = UniqueResultBank::get_instance();

    auto detail_any = bank.get_node_detail(node_id);
    if (!detail_any.has_value()) {
        LogError << "failed to query" << VAR(node_id);
        return false;
    }
    auto detail = std::any_cast<NodeDetail>(detail_any);

    name = detail.name;
    reco_id = detail.hits.reco_uid;
    completed = detail.status == NodeStatus::Success;

    return true;
}

bool PipelineTask::query_task_detail(
    MaaTaskId task_id,
    std::string& entry,
    std::vector<MaaNodeId>& node_id_list)
{
    const auto& bank = UniqueResultBank::get_instance();

    auto detail_any = bank.get_task_detail(task_id);
    if (!detail_any.has_value()) {
        LogError << "failed to query" << VAR(task_id);
        return false;
    }
    auto detail = std::any_cast<TaskDetail>(detail_any);

    entry = detail.entry;
    node_id_list = detail.node_ids;

    return true;
}

PipelineTask::NodeStatus PipelineTask::find_first_and_run(
    const std::vector<std::string>& list,
    std::chrono::milliseconds timeout,
    /*out*/ MAA_RES_NS::TaskData& found_data)
{
    HitDetail hits;

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        auto find_opt = find_first(list);
        if (find_opt) {
            hits = *std::move(find_opt);
            break;
        }

        if (need_to_stop()) {
            LogInfo << "Task interrupted" << VAR(pre_hit_task_);
            return NodeStatus::Interrupted;
        }

        if (std::chrono::steady_clock::now() - start_time > timeout) {
            LogInfo << "Task timeout" << VAR(pre_hit_task_) << VAR(timeout);
            return NodeStatus::Timeout;
        }
    }

    LogInfo << "Task hit:" << hits.task_data.name << VAR(hits.reco_uid) << VAR(hits.reco_hit)
            << VAR(hits.reco_detail);

    hit_cache_.insert_or_assign(hits.task_data.name, hits.reco_hit);

    auto run_ret = run_task(hits);

    found_data = std::move(hits.task_data);

    return run_ret;
}

std::optional<PipelineTask::HitDetail>
    PipelineTask::find_first(const std::vector<std::string>& list)
{
    if (!controller()) {
        LogError << "Controller not binded";
        return std::nullopt;
    }
    if (need_to_stop()) {
        LogInfo << "Task interrupted" << VAR(pre_hit_task_);
        return std::nullopt;
    }

    LogFunc << VAR(pre_hit_task_) << VAR(list);

    cv::Mat image = controller()->screencap();

    if (image.empty()) {
        LogError << "Image is empty";
        return std::nullopt;
    }

    if (need_to_stop()) {
        LogInfo << "Task interrupted" << VAR(pre_hit_task_);
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
    HitDetail result;

    for (const std::string& name : list) {
        LogDebug << "recognize:" << name;

        const auto& task_data = data_mgr_.get_task_data(name);
        if (!task_data.enabled) {
            LogDebug << "Task disabled:" << name;
            continue;
        }

        auto reco = recognizer.recognize(image, task_data);

        if (debug_mode()) {
            json::value cb_detail = basic_info() | reco_result_to_json(name, reco);
            notify(MaaMsg_Task_Debug_RecognitionResult, cb_detail);
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
        if (debug_mode()) {
            json::value detail = basic_info()
                                 | json::object {
                                       { "list", json::array(list) },
                                   };
            notify(MaaMsg_Task_Debug_MissAll, detail);
        }

        return std::nullopt;
    }

    if (debug_mode()) {
        json::value cb_detail = basic_info() | hit_detail_to_json(result);
        notify(MaaMsg_Task_Debug_Hit, cb_detail);
    }

    return result;
}

PipelineTask::NodeStatus PipelineTask::run_task(const HitDetail& hits)
{
    if (need_to_stop()) {
        LogInfo << "Task interrupted" << VAR(pre_hit_task_);
        return NodeStatus::Interrupted;
    }

    Actuator actuator(inst_, hit_cache_);

    const std::string& name = hits.task_data.name;
    uint64_t& run_times = run_times_map_[name];

    NodeDetail node_detail { .name = name, .hits = hits };

    if (debug_mode() || hits.task_data.focus) {
        json::value cb_detail = basic_info() | node_detail_to_json(node_detail);
        if (debug_mode()) {
            notify(MaaMsg_Task_Debug_ReadyToRun, cb_detail);
        }
        if (hits.task_data.focus) {
            notify(MaaMsg_Task_Focus_ReadyToRun, cb_detail);
        }
    }

    if (hits.task_data.times_limit <= run_times) {
        LogInfo << "Task runout:" << name;

        node_detail.status = NodeStatus::Runout;

        add_node_detail(node_detail.node_id, node_detail);

        if (debug_mode() || hits.task_data.focus) {
            json::value cb_detail = basic_info() | node_detail_to_json(node_detail);
            if (debug_mode()) {
                notify(MaaMsg_Task_Debug_Runout, cb_detail);
            }
            if (hits.task_data.focus) {
                notify(MaaMsg_Task_Focus_Runout, cb_detail);
            }
        }

        return NodeStatus::Runout;
    }

    auto ret = actuator.run(hits.reco_hit, hits.reco_detail, hits.task_data);

    ++run_times;

    node_detail.status = NodeStatus::Success;

    add_node_detail(node_detail.node_id, node_detail);

    if (debug_mode() || hits.task_data.focus) {
        json::value cb_detail = basic_info() | node_detail_to_json(node_detail);
        if (debug_mode()) {
            notify(MaaMsg_Task_Debug_Completed, cb_detail);
        }
        if (hits.task_data.focus) {
            notify(MaaMsg_Task_Focus_Completed, cb_detail);
        }
    }

    return ret ? NodeStatus::Success : NodeStatus::InternalError;
}

void PipelineTask::add_node_detail(int64_t node_id, NodeDetail detail)
{
    auto& bank = UniqueResultBank::get_instance();
    bank.add_node_detail(node_id, detail);

    TaskDetail task_detail { .entry = entry_ };
    std::any task_detail_any = bank.get_task_detail(task_id_);
    if (task_detail_any.has_value()) {
        task_detail = std::any_cast<TaskDetail>(task_detail_any);
    }
    task_detail.node_ids.emplace_back(node_id);
    bank.add_task_detail(task_id_, task_detail);
}

bool PipelineTask::debug_mode() const
{
    return GlobalOptionMgr::get_instance().debug_message();
}

json::object PipelineTask::basic_info()
{
    return {
        { "task_id", task_id_ },
        { "entry", entry() },
        { "hash", resource() ? resource()->get_hash() : std::string() },
        { "uuid", controller() ? controller()->get_uuid() : std::string() },
        { "pre_hit_task", pre_hit_task_ },
    };
}

json::object
    PipelineTask::reco_result_to_json(const std::string& name, const Recognizer::Result& res)
{
    return {
        { "name", name },
        { "recognition",
          {
              { "id", res.uid },
              { "box", res.hit ? json::value(*res.hit) : json::value(nullptr) },
              { "detail", res.detail },
              { "hit", res.hit.has_value() },
          } },
    };
}

json::object PipelineTask::hit_detail_to_json(const HitDetail& detail)
{
    return {
        { "name", detail.task_data.name },
        { "recognition",
          {
              { "id", detail.reco_uid },
              { "box", detail.reco_hit },
              { "detail", detail.reco_detail },
              { "hit", true },
          } },
    };
}

json::object PipelineTask::node_detail_to_json(const NodeDetail& detail)
{
    return hit_detail_to_json(detail.hits)
           | json::object {
                 { "node_id", detail.node_id },
                 { "status", static_cast<int>(detail.status) },
             };
}

MAA_TASK_NS_END
