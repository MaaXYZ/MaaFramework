#include "PipelineTask.h"

#include <stack>

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"
#include "Utils/JsonExt.hpp"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

bool PipelineTask::run()
{
    if (!context_) {
        LogError << "context is null";
        return {};
    }

    LogFunc << VAR(entry_);

    std::stack<PipelineData::NextList> goto_stack;
    PipelineData::NextList next = { entry_ };
    PipelineData::NextList catch_next;

    while (!next.empty() && !need_to_stop_) {
        PipelineData current = context_->get_pipeline_data(cur_task_);
        auto [node_result, is_catch] = run_reco_and_action(next, catch_next, current.next_timeout);

        if (!node_result.completed) {
            LogError << "Run task failed:" << next;
            return false;
        }

        PipelineData hit_task = context_->get_pipeline_data(node_result.name);
        if (is_catch || hit_task.is_sub) { // for compatibility with v1.x
            const auto& ref = goto_stack.emplace(next);
            LogDebug << "push goto_stack:" << hit_task.name << ref;
        }

        next = hit_task.next;
        catch_next = hit_task.catch_next;

        if (next.empty() && !goto_stack.empty()) {
            next = std::move(goto_stack.top());
            goto_stack.pop();
            LogDebug << "pop goto_stack:" << next;
        }

        cur_task_ = hit_task.name;
    }
    return true;
}

void PipelineTask::post_stop()
{
    need_to_stop_ = true;
}

std::pair<NodeDetail, /* is catch */ bool> PipelineTask::run_reco_and_action(
    const PipelineData::NextList& next,
    const PipelineData::NextList& catch_next,
    const std::chrono::milliseconds& timeout)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }

    RecoResult reco;
    bool is_catch = false;

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        cv::Mat image = screencap();
        reco = run_recogintion(image, next);
        if (reco.box) { // hit
            is_catch = false;
            break;
        }

        reco = run_recogintion(image, catch_next);
        if (reco.box) { // hit
            is_catch = true;
            break;
        }

        if (need_to_stop_) {
            LogError << "Task interrupted" << VAR(cur_task_);
            return {};
        }

        if (std::chrono::steady_clock::now() - start_time > timeout) {
            LogError << "Task timeout" << VAR(cur_task_) << VAR(timeout);
            return {};
        }
    }

    if (!reco.box) {
        return {};
    }

    auto node_detail = run_action(reco);

    return std::make_pair(node_detail, is_catch);
}

MAA_TASK_NS_END
