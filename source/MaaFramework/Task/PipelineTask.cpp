#include "PipelineTask.h"

#include <stack>

#include "Controller/ControllerAgent.h"
#include "Global/OptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "Resource/PipelineDumper.h"
#include "Resource/PipelineParser.h"
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

    std::stack<std::string> jumpback_stack;

    // there is no pretask for the entry, so we use the entry itself
    auto begin_opt = context_->get_pipeline_data(entry_);
    if (!begin_opt) {
        LogError << "get_pipeline_data failed, task not exist" << VAR(entry_);
        return false;
    }

    PipelineData node = std::move(*begin_opt);
    std::vector<MAA_RES_NS::NodeAttr> next = { { .name = entry_ } };

    bool error_handling = false;

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
            std::string pre_node_name = node.name;
            node = std::move(*hit_opt);

            auto it = std::ranges::find_if(next, [&](const MAA_RES_NS::NodeAttr& n) {
                auto data_opt = context_->get_pipeline_data(n);
                return data_opt && data_opt->name == node_detail.name;
            });
            if (it != next.end() && it->jump_back) {
                LogInfo << "push jumpback_stack:" << pre_node_name;
                jumpback_stack.emplace(pre_node_name);
            }

            if (node_detail.completed) {
                next = node.next;
            }
            else { // 动作执行失败了
                LogWarn << "node not completed, handle error" << VAR(node.name);
                error_handling = true;
                next = node.on_error;
                save_on_error(node.name);
            }
        }
        else if (error_handling) {
            LogError << "error handling loop detected" << VAR(node.name);
            next.clear();
            save_on_error(node.name);
        }
        else {
            LogWarn << "invalid node id, handle error" << VAR(node.name);
            error_handling = true;
            next = node.on_error;
            save_on_error(node.name);
        }

        if (next.empty() && !jumpback_stack.empty()) {
            auto top = std::move(jumpback_stack.top());
            LogInfo << "pop jumpback_stack:" << top;
            jumpback_stack.pop();

            auto top_opt = context_->get_pipeline_data(top);
            if (!top_opt) {
                LogError << "get_pipeline_data failed, task not exist" << VAR(top);
                return false;
            }
            node = std::move(*top_opt);

            next = node.next;
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

NodeDetail PipelineTask::run_next(const std::vector<MAA_RES_NS::NodeAttr>& next, const PipelineData& pretask)
{
    if (!context_) {
        LogError << "context is null";
        return {};
    }

    bool valid = std::ranges::any_of(next, [&](const MAA_RES_NS::NodeAttr& node) {
        auto data_opt = context_->get_pipeline_data(node);
        return data_opt && data_opt->enabled;
    });
    if (!valid) {
        LogInfo << "no valid/enabled node in next" << VAR(next);
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

    json::value node_cb_detail {
        { "task_id", task_id() },
        { "node_id", node_id },
        { "name", cur_node_ },
        { "focus", cur_node.focus },
    };

    notify(MaaMsg_Node_PipelineNode_Starting, node_cb_detail);

    while (!context_->need_to_stop()) {
        auto current_clock = std::chrono::steady_clock::now();
        cv::Mat image = screencap();

        RecoResult reco = recognize_list(image, next);

        if (context_->need_to_stop()) {
            LogWarn << "need_to_stop" << VAR(pretask.name);
            break;
        }

        if (!reco.box) {
            if (duration_since(start_clock) > pretask.reco_timeout) {
                LogWarn << "Task timeout" << VAR(pretask.name) << VAR(duration_since(start_clock)) << VAR(pretask.reco_timeout);
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

            notify(MaaMsg_Node_PipelineNode_Failed, node_cb_detail);

            return {};
        }

        auto act = run_action(reco, *hit_opt);

        for (const auto& anchor : hit_opt->anchor) {
            context_->set_anchor(anchor, hit_name);
        }

        NodeDetail result {
            .node_id = node_id,
            .name = hit_name,
            .reco_id = reco.reco_id,
            .action_id = act.action_id,
            .completed = act.success,
        };

        LogInfo << "PipelineTask node done" << VAR(result) << VAR(task_id_);
        set_node_detail(result.node_id, result);

        node_cb_detail["node_details"] = result;
        node_cb_detail["reco_details"] = reco;
        node_cb_detail["action_details"] = act;

        notify(act.success ? MaaMsg_Node_PipelineNode_Succeeded : MaaMsg_Node_PipelineNode_Failed, node_cb_detail);

        return result;
    }

    NodeDetail result {
        .node_id = node_id,
        .completed = false,
    };
    LogWarn << "PipelineTask bad next" << VAR(result) << VAR(task_id_);
    set_node_detail(result.node_id, result);

    notify(MaaMsg_Node_PipelineNode_Failed, node_cb_detail);

    return result;
}

RecoResult PipelineTask::recognize_list(const cv::Mat& image, const std::vector<MAA_RES_NS::NodeAttr>& list)
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
        { "list", list },
        { "focus", cur_node.focus },
    };

    notify(MaaMsg_Node_NextList_Starting, reco_list_cb_detail);

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

        notify(MaaMsg_Node_NextList_Succeeded, reco_list_cb_detail);

        return result;
    }

    notify(MaaMsg_Node_NextList_Failed, reco_list_cb_detail);

    return {};
}

void PipelineTask::save_on_error(const std::string& node_name)
{
    const auto& option = MAA_GLOBAL_NS::OptionMgr::get_instance();

    if (!option.save_on_error()) {
        return;
    }

    if (!controller()) {
        LogError << "controller is null";
        return;
    }

    auto image = controller()->cached_image();
    if (image.empty()) {
        LogError << "cached_image is empty";
        return;
    }

    std::string filename = std::format("{}_{}.png", node_name, format_now_for_filename());
    auto filepath = option.log_dir() / "on_error" / path(filename);
    imwrite(filepath, image);
    LogInfo << "save on error to" << filepath;
}

MAA_TASK_NS_END
