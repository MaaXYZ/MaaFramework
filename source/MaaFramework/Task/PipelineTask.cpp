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
    PipelineData::NextList interrupt;

    while (!next.empty() && !need_to_stop_) {
        PipelineData current = context_->get_pipeline_data(cur_task_);
        auto [node_result, is_interrupt] = run_reco_and_action(next, interrupt, current);

        if (!node_result.completed) {
            LogError << "Run task failed:" << next;
            return false;
        }

        PipelineData hit_task = context_->get_pipeline_data(node_result.name);
        if (is_interrupt || hit_task.is_sub) { // for compatibility with v1.x
            const auto& ref = goto_stack.emplace(next);
            LogDebug << "push goto_stack:" << hit_task.name << ref;
        }

        next = hit_task.next;
        interrupt = hit_task.interrupt;

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

std::pair<NodeDetail, /* is interrupt */ bool> PipelineTask::run_reco_and_action(
    const PipelineData::NextList& next,
    const PipelineData::NextList& interrupt,
    const PipelineData& pp_data)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }

    RecoResult reco;
    bool is_interrupt = false;

    const auto start_clock = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point current_clock;
    while (true) {
        current_clock = std::chrono::steady_clock::now();
        cv::Mat image = screencap();

        reco = run_recogintion(image, next);
        if (reco.box) { // hit
            is_interrupt = false;
            break;
        }

        reco = run_recogintion(image, interrupt);
        if (reco.box) { // hit
            is_interrupt = true;
            break;
        }

        if (need_to_stop_) {
            LogError << "need_to_stop" << VAR(cur_task_);
            return {};
        }

        if (std::chrono::steady_clock::now() - start_clock > pp_data.reco_timeout) {
            LogError << "Task timeout" << VAR(cur_task_) << VAR(pp_data.reco_timeout);
            return {};
        }

        std::this_thread::sleep_until(current_clock + pp_data.rate_limit);
    }

    if (!reco.box) {
        return {};
    }

    auto node_detail = run_action(reco);

    return std::make_pair(node_detail, is_interrupt);
}

MAA_TASK_NS_END
