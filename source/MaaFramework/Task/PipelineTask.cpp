#include "PipelineTask.h"

#include <stack>

#include "Controller/ControllerAgent.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

bool PipelineTask::run()
{
    if (!context_) {
        LogError << "context is null";
        return false;
    }

    LogFunc << VAR(entry_) << VAR(task_id_);

    std::stack<std::string> task_stack;

    // there is no pretask for the entry, so we use the entry itself
    auto begin_opt = context_->get_pipeline_data(entry_);
    if (!begin_opt) {
        LogError << "get_pipeline_data failed, task not exist" << VAR(entry_);
        return false;
    }

    PipelineData node = std::move(*begin_opt);
    PipelineData::NextList next = { entry_ };
    PipelineData::NextList interrupt;
    bool error_handling = false;

    while (!next.empty() && !context_->need_to_stop()) {
        cur_node_ = node.name;

        size_t next_size = next.size();
        PipelineData::NextList list = std::move(next);
        list.insert(list.end(), std::make_move_iterator(interrupt.begin()), std::make_move_iterator(interrupt.end()));

        auto node_detail = run_next(list, node);

        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop" << VAR(node.name);
            return true;
        }

        if (node_detail.node_id != MaaInvalidId) {
            error_handling = false;

            // 如果 list 里有同名任务，返回值也一定是第一个。同名任务第一个匹配上了后面肯定也会匹配上（除非 Custom 写了一些什么逻辑）
            // 且 PipelineChecker::check_all_next_list 保证了 next + interrupt 中没有同名任务
            auto pos = std::ranges::find(list, node_detail.name) - list.begin();
            bool is_interrupt = static_cast<size_t>(pos) >= next_size;
            auto hit_opt = context_->get_pipeline_data(node_detail.name);
            if (!hit_opt) {
                LogError << "get_pipeline_data failed, task not exist" << VAR(node_detail.name);
                return false;
            }
            PipelineData hit_node = std::move(*hit_opt);

            if (is_interrupt || hit_node.is_sub) { // for compatibility with v1.x
                LogInfo << "push task_stack:" << node.name;
                task_stack.emplace(node.name);
            }

            if (node_detail.completed) {
                next = hit_node.next;
                interrupt = hit_node.interrupt;
            }
            else {
                LogWarn << "node not completed, handle error" << VAR(node_detail.name);
                error_handling = true;
                next = hit_node.on_error;
                interrupt.clear();
            }
            node = std::move(hit_node);
        }
        else if (error_handling) {
            LogError << "error handling loop detected" << VAR(node.name);
            next.clear();
            interrupt.clear();
        }
        else {
            LogInfo << "invalid node id, handle error" << VAR(node.name);
            error_handling = true;
            next = node.on_error;
            interrupt.clear();
        }

        if (next.empty() && !task_stack.empty()) {
            auto top = std::move(task_stack.top());
            LogInfo << "pop task_stack:" << top;
            task_stack.pop();

            auto top_opt = context_->get_pipeline_data(top);
            if (!top_opt) {
                LogError << "get_pipeline_data failed, task not exist" << VAR(top);
                return false;
            }
            node = std::move(*top_opt);
            next = node.next;
            interrupt = node.interrupt;
        }
    }

    return !error_handling;
}

void PipelineTask::post_stop()
{
    if (!context_) {
        LogError << "context is null";
        return;
    }
    context_->need_to_stop() = true;
}

NodeDetail PipelineTask::run_next(const PipelineData::NextList& list, const PipelineData& pretask)
{
    if (!context_) {
        LogError << "context is null";
        return {};
    }

    bool valid = std::ranges::any_of(list, [&](const std::string& name) {
        auto data_opt = context_->get_pipeline_data(name);
        return data_opt && data_opt->enabled;
    });
    if (!valid) {
        LogInfo << "no valid/enabled node in list" << VAR(list);
        return {};
    }

    auto node_id = generate_node_id();
    const auto start_clock = std::chrono::steady_clock::now();

    auto cur_opt = context_->get_pipeline_data(cur_node_);
    if (!cur_opt) {
        LogError << "get_pipeline_data failed, node not exist" << VAR(cur_node_);
        return {};
    }

    const auto& cur_node = *cur_opt;

    const json::value node_cb_detail {
        { "task_id", task_id() },
        { "node_id", node_id },
        { "name", cur_node_ },
        { "focus", cur_node.focus },
    };

    if (debug_mode() || !cur_node.focus.is_null()) {
        notify(MaaMsg_Context_PipelineNode_Starting, node_cb_detail);
    }

    while (!context_->need_to_stop()) {
        auto current_clock = std::chrono::steady_clock::now();
        cv::Mat image = screencap();

        RecoResult reco = recognize_list(image, list);

        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop" << VAR(pretask.name);
            break;
        }

        if (!reco.box) {
            if (duration_since(start_clock) > pretask.reco_timeout) {
                LogError << "Task timeout" << VAR(pretask.name) << VAR(duration_since(start_clock)) << VAR(pretask.reco_timeout)
                         << VAR(list);
                break;
            }

            LogDebug << "sleep_until" << VAR(pretask.rate_limit);
            std::this_thread::sleep_until(current_clock + pretask.rate_limit);

            continue;
        }

        std::string hit_name = reco.name;
        auto hit_opt = context_->get_pipeline_data(hit_name);
        if (!hit_opt) {
            LogError << "get_pipeline_data failed, node not exist" << VAR(hit_name);

            if (debug_mode() || !cur_node.focus.is_null()) {
                notify(MaaMsg_Context_PipelineNode_Failed, node_cb_detail);
            }

            return {};
        }

        auto act = run_action(reco, *hit_opt);

        NodeDetail result {
            .node_id = node_id,
            .name = hit_name,
            .reco_id = reco.reco_id,
            .action_id = act.action_id,
            .completed = act.success,
        };
        LogInfo << "PipelineTask node done" << VAR(result) << VAR(task_id_);
        set_node_detail(result.node_id, result);

        if (debug_mode() || !cur_node.focus.is_null()) {
            notify(act.success ? MaaMsg_Context_PipelineNode_Succeeded : MaaMsg_Context_PipelineNode_Failed, node_cb_detail);
        }

        return result;
    }

    NodeDetail result {
        .node_id = node_id,
        .completed = false,
    };
    LogError << "PipelineTask bad next" << VAR(result) << VAR(task_id_);
    set_node_detail(result.node_id, result);

    if (debug_mode() || !cur_node.focus.is_null()) {
        notify(MaaMsg_Context_PipelineNode_Failed, node_cb_detail);
    }

    return result;
}

RecoResult PipelineTask::recognize_list(const cv::Mat& image, const PipelineData::NextList& list)
{
    LogFunc << VAR(cur_node_) << VAR(list);

    if (!context_) {
        LogError << "context is null";
        return {};
    }

    if (image.empty()) {
        LogError << "Image is empty";
        return {};
    }

    auto cur_opt = context_->get_pipeline_data(cur_node_);
    if (!cur_opt) {
        LogError << "get_pipeline_data failed, node not exist" << VAR(cur_node_);
        return {};
    }

    const auto& cur_node = *cur_opt;

    const json::value reco_list_cb_detail {
        { "task_id", task_id() },
        { "name", cur_node_ },
        { "list", json::array(list) },
        { "focus", cur_node.focus },
    };

    if (debug_mode() || !cur_node.focus.is_null()) {
        notify(MaaMsg_Node_NextList_Starting, reco_list_cb_detail);
    }

    for (const auto& node : list) {
        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop";
            break;
        }

        auto node_opt = context_->get_pipeline_data(node);
        if (!node_opt) {
            LogError << "get_pipeline_data failed, node not exist" << VAR(node);
            continue;
        }
        const auto& pipeline_data = *node_opt;

        RecoResult result = run_recognition(image, pipeline_data);

        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop";
            break;
        }
        if (!result.box) {
            continue;
        }

        if (debug_mode() || !cur_node.focus.is_null()) {
            notify(MaaMsg_Node_NextList_Succeeded, reco_list_cb_detail);
        }

        return result;
    }

    if (debug_mode() || !cur_node.focus.is_null()) {
        notify(MaaMsg_Node_NextList_Failed, reco_list_cb_detail);
    }

    return {};
}

MAA_TASK_NS_END
