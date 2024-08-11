#include "PipelineTask.h"

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "Global/UniqueResultBank.h"
#include "Instance/InstanceCache.h"
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

void PipelineTask::post_stop()
{
    need_to_stop_ = true;
}

bool PipelineTask::on_set_param(int64_t task_id, const json::value& param)
{
    if (task_id != task_id_) {
        return true;
    }

    return set_param(param);
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

    std::stack<TaskData::NextList> goto_stack;
    TaskData::NextList next_list = { TaskData::NextObject { .name = entry_ } };

    while (!next_list.empty() && !need_to_stop()) {
        auto iter = find_first_and_run(next_list);

        if (iter == next_list.cend()) {
            LogError << "Run task failed:" << next_list;
            return false;
        }
        const TaskData::NextObject& hit_object = *iter;
        TaskData hit_task = data_mgr_.get_task_data(hit_object.name);

        switch (hit_object.then_goto) {
        case TaskData::NextObject::ThenGotoLabel::None:
            if (hit_task.is_sub) { // for compatibility with v1.x
                const auto& ref = goto_stack.emplace(next_list);
                LogDebug << "push then_goto is_sub:" << hit_object.name << ref;
            }
            break;
        case TaskData::NextObject::ThenGotoLabel::Head: {
            const auto& ref = goto_stack.emplace(next_list);
            LogDebug << "push then_goto head:" << hit_object.name << ref;
        } break;
        case TaskData::NextObject::ThenGotoLabel::Current: {
            const auto& ref = goto_stack.emplace(iter, next_list.cend());
            LogDebug << "push then_goto current:" << hit_object.name << ref;
        } break;
        case TaskData::NextObject::ThenGotoLabel::Following: {
            const auto& ref = goto_stack.emplace(iter + 1, next_list.cend());
            LogDebug << "push then_goto following:" << hit_object.name << ref;
        } break;
        }

        next_list = hit_task.next;

        if (next_list.empty() && !goto_stack.empty()) {
            next_list = std::move(goto_stack.top());
            goto_stack.pop();
            LogDebug << "pop then_goto:" << next_list;
        }

        pre_hit_task_ = hit_task.name;
    }
    return true;
}

bool PipelineTask::run_recognition_only()
{
    LogFunc << VAR(entry_);

    TaskData::NextList next_list = { TaskData::NextObject { .name = entry_ } };
    HitDetail hit_detail;
    auto iter = find_first(next_list, hit_detail);
    bool hit = iter != next_list.cend();
    if (hit) {
        NodeDetail node { .hit = std::move(hit_detail), .status = NodeStatus::OnlyRecognized };
        auto nid = node.node_id;
        add_node_detail(nid, std::move(node));
    }

    return hit;
}

bool PipelineTask::run_action_only()
{
    LogFunc << VAR(entry_);

    HitDetail fake_hit { .task_data = data_mgr_.get_task_data(entry_) };
    return run_task(fake_hit);
}

bool PipelineTask::set_param(const json::value& param)
{
    return data_mgr_.set_param(param);
}

bool PipelineTask::query_node_detail(MaaNodeId node_id, std::string& name, MaaRecoId& reco_id, bool& completed)
{
    const auto& bank = UniqueResultBank::get_instance();

    auto detail_any = bank.get_node_detail(node_id);
    if (!detail_any.has_value()) {
        LogError << "failed to query" << VAR(node_id);
        return false;
    }
    auto detail = std::any_cast<NodeDetail>(detail_any);

    name = detail.name;
    reco_id = detail.hit.reco_uid;
    completed = detail.status == NodeStatus::RunCompleted;

    return true;
}

bool PipelineTask::query_task_detail(MaaTaskId task_id, std::string& entry, std::vector<MaaNodeId>& node_id_list)
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

PipelineTask::NextIter PipelineTask::find_first_and_run(const TaskData::NextList& list)
{
    const NextIter NotFound = list.cend();

    if (!inst_ || !inst_->cache()) {
        LogError << "Inst or cache is null";
        return NotFound;
    }

    const auto timeout = GlobalOptionMgr::get_instance().pipeline_timeout();

    auto iter = list.cend();
    HitDetail hit_detail;

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        iter = find_first(list, hit_detail);
        if (iter != list.cend()) {
            // found
            break;
        }

        if (need_to_stop()) {
            LogError << "Task interrupted" << VAR(pre_hit_task_);
            return NotFound;
        }

        if (std::chrono::steady_clock::now() - start_time > timeout) {
            LogError << "Task timeout" << VAR(pre_hit_task_) << VAR(timeout);
            return NotFound;
        }
    }

    LogInfo << "Task hit:" << hit_detail.task_data.name << VAR(hit_detail.reco_uid) << VAR(hit_detail.reco_hit)
            << VAR(hit_detail.reco_detail.to_string());

    inst_->cache()->set_pre_box(hit_detail.task_data.name, hit_detail.reco_hit);

    bool run_ret = run_task(hit_detail);
    return run_ret ? iter : NotFound;
}

PipelineTask::NextIter PipelineTask::find_first(const TaskData::NextList& list, HitDetail& hit_detail)
{
    const NextIter NotFound = list.cend();

    if (!controller()) {
        LogError << "Controller not binded";
        return NotFound;
    }
    if (need_to_stop()) {
        LogInfo << "Task interrupted" << VAR(pre_hit_task_);
        return NotFound;
    }

    LogFunc << VAR(pre_hit_task_) << VAR(list);

    cv::Mat image = controller()->screencap();

    if (image.empty()) {
        LogError << "Image is empty";
        return NotFound;
    }

    if (need_to_stop()) {
        LogInfo << "Task interrupted" << VAR(pre_hit_task_);
        return NotFound;
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

    auto iter = list.cbegin();

    for (; iter != list.cend(); ++iter) {
        const std::string& name = iter->name;
        uint64_t& hit_times = hit_times_map_[name];

        const auto& task_data = data_mgr_.get_task_data(name);
        if (!task_data.enabled || task_data.hit_limit <= hit_times) {
            LogDebug << "Task disabled or hit over limit" << name << VAR(task_data.enabled) << VAR(hit_times) << VAR(task_data.hit_limit);
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
        ++hit_times;
        hit_detail = { .reco_uid = reco.uid,
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

        return NotFound;
    }

    if (debug_mode()) {
        json::value cb_detail = basic_info() | hit_detail_to_json(hit_detail);
        notify(MaaMsg_Task_Debug_Hit, cb_detail);
    }

    return iter;
}

bool PipelineTask::run_task(const HitDetail& hit)
{
    if (need_to_stop()) {
        LogInfo << "Task interrupted" << VAR(pre_hit_task_);
        return false;
    }

    Actuator actuator(inst_);

    const std::string& name = hit.task_data.name;

    NodeDetail node_detail { .name = name, .hit = hit };

    if (debug_mode() || hit.task_data.focus) {
        json::value cb_detail = basic_info() | node_detail_to_json(node_detail);
        if (debug_mode()) {
            notify(MaaMsg_Task_Debug_ReadyToRun, cb_detail);
        }
        if (hit.task_data.focus) {
            notify(MaaMsg_Task_Focus_ReadyToRun, cb_detail);
        }
    }

    bool ret = actuator.run(hit.reco_hit, hit.reco_detail, hit.task_data);

    node_detail.status = ret ? NodeStatus::RunCompleted : NodeStatus::None;
    add_node_detail(node_detail.node_id, node_detail);

    if (debug_mode() || hit.task_data.focus) {
        json::value cb_detail = basic_info() | node_detail_to_json(node_detail);
        if (debug_mode()) {
            notify(MaaMsg_Task_Debug_Completed, cb_detail);
        }
        if (hit.task_data.focus) {
            notify(MaaMsg_Task_Focus_Completed, cb_detail);
        }
    }

    return ret;
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

json::object PipelineTask::reco_result_to_json(const std::string& name, const Recognizer::Result& res)
{
    return {
        { "name", name },
        { "recognition",
          {
              { "id", res.uid },
              { "box", res.hit ? json::value(*res.hit) : json::value(nullptr) },
              { "detail", res.detail },
              { "hit_task", res.hit.has_value() },
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
              { "hit_task", true },
          } },
    };
}

json::object PipelineTask::node_detail_to_json(const NodeDetail& detail)
{
    return hit_detail_to_json(detail.hit)
           | json::object {
                 { "node_id", detail.node_id },
                 { "status", static_cast<int>(detail.status) },
             };
}

MAA_TASK_NS_END
