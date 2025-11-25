#include "PipelineTask.h"

#include <stack>

#include "Controller/ControllerAgent.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

template <typename T>
std::vector<T> operator+(const std::vector<T>& lhs, const std::vector<T>& rhs)
{
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
    return lhs;
}

bool PipelineTask::run()
{
    if (!context_) {
        LogError << "context is null";
        return false;
    }

    LogFunc << VAR(entry_) << VAR(task_id_);

    std::stack<std::string> interrupt_stack;

    // there is no pretask for the entry, so we use the entry itself
    auto begin_opt = context_->get_pipeline_data(entry_);
    if (!begin_opt) {
        LogError << "get_pipeline_data failed, task not exist" << VAR(entry_);
        return false;
    }

    PipelineData node = std::move(*begin_opt);
    PipelineData::NextList next = { entry_ };
    size_t interrupt_pos = SIZE_MAX;

    std::map<std::string, std::string> all_checkpoints;

    bool error_handling = false;

    auto make_jumpnodes = [&](const PipelineData& pd) {
        std::vector<std::string> jump_nodes;
        for (const std::string& jumpback : pd.jumpback) {
            auto it = all_checkpoints.find(jumpback);
            if (it != all_checkpoints.end()) {
                jump_nodes.emplace_back(it->second);
            }
        }
        return jump_nodes;
    };

    while (!next.empty() && !context_->need_to_stop()) {
        cur_node_ = node.name;
        auto node_detail = run_next(next, node);

        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop" << VAR(node.name);
            return true;
        }

        // 识别命中新节点
        if (node_detail.reco_id != MaaInvalidId) {
            error_handling = false;
            auto hit_opt = context_->get_pipeline_data(node_detail.name);
            if (!hit_opt) {
                LogError << "get_pipeline_data failed, task not exist" << VAR(node_detail.name);
                return false;
            }
            PipelineData hit_node = std::move(*hit_opt);

            for (const std::string& checkpoint : hit_node.checkpoint) {
                all_checkpoints.insert_or_assign(checkpoint, hit_node.name);
                LogInfo << "set checkpoint" << VAR(checkpoint) << VAR(hit_node.name) << VAR(all_checkpoints);
            }

            // 如果 next 里有同名任务，返回值也一定是第一个。同名任务第一个匹配上了后面肯定也会匹配上（除非 Custom 写了一些什么逻辑）
            // 且 PipelineChecker::check_all_next_list 保证了 next + interrupt 中没有同名任务
            auto pos = std::ranges::find(next, node_detail.name) - next.begin();
            bool is_interrupt = static_cast<size_t>(pos) >= interrupt_pos;
            if (is_interrupt || hit_node.is_sub) { // for compatibility with v1.x
                LogInfo << "push interrupt_stack:" << node.name;
                interrupt_stack.emplace(node.name);
            }

            if (node_detail.completed) {
                if (!hit_node.jumpback.empty()) {
                    auto jump_nodes = make_jumpnodes(hit_node);
                    LogInfo << "jumpback to checkpoint" << VAR(hit_node.name) << VAR(hit_node.jumpback) << VAR(jump_nodes);
                    next = jump_nodes;
                }
                else {
                    next = hit_node.next + hit_node.interrupt;
                    interrupt_pos = hit_node.next.size();
                }
            }
            else { // 动作执行失败了
                LogWarn << "node not completed, handle error" << VAR(node_detail.name);
                error_handling = true;
                next = hit_node.on_error;
                interrupt_pos = SIZE_MAX;
            }
            node = std::move(hit_node);
        }
        else if (error_handling) {
            LogError << "error handling loop detected" << VAR(node.name);
            next.clear();
            interrupt_pos = SIZE_MAX;
        }
        else {
            LogInfo << "invalid node id, handle error" << VAR(node.name);
            error_handling = true;
            next = node.on_error;
            interrupt_pos = next.size();
        }

        if (next.empty() && !interrupt_stack.empty()) {
            auto top = std::move(interrupt_stack.top());
            LogInfo << "pop interrupt_stack:" << top;
            interrupt_stack.pop();

            auto top_opt = context_->get_pipeline_data(top);
            if (!top_opt) {
                LogError << "get_pipeline_data failed, task not exist" << VAR(top);
                return false;
            }
            node = std::move(*top_opt);

            if (!node.jumpback.empty()) {
                auto jump_nodes = make_jumpnodes(node);
                LogInfo << "jumpback to checkpoint" << VAR(node.name) << VAR(node.jumpback) << VAR(jump_nodes);
                next = jump_nodes;
            }
            else {
                next = node.next + node.interrupt;
                interrupt_pos = node.next.size();
            }
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
        notify(MaaMsg_Node_PipelineNode_Starting, node_cb_detail);
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
                notify(MaaMsg_Node_PipelineNode_Failed, node_cb_detail);
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
            notify(act.success ? MaaMsg_Node_PipelineNode_Succeeded : MaaMsg_Node_PipelineNode_Failed, node_cb_detail);
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
        notify(MaaMsg_Node_PipelineNode_Failed, node_cb_detail);
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
