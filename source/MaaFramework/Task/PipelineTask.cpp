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
        return false;
    }

    LogFunc << VAR(entry_);

    std::stack<std::string> task_stack;

    // there is no pretask for the entry, so we use the entry itself
    PipelineData current = context_->get_pipeline_data(entry_);
    PipelineData::NextList next = { entry_ };
    PipelineData::NextList interrupt;
    bool error_handling = false;

    while (!next.empty() && !need_to_stop_) {
        cur_task_ = current.name;
        auto [node_detail, is_interrupt] = run_reco_and_action(next, interrupt, current);
        if (need_to_stop_) {
            LogError << "need_to_stop" << VAR(current.name);
            return true;
        }

        if (node_detail.completed) {
            error_handling = false;
            PipelineData hit_task = context_->get_pipeline_data(node_detail.name);
            if (is_interrupt || hit_task.is_sub) { // for compatibility with v1.x
                LogInfo << "push task_stack:" << current.name;
                task_stack.emplace(current.name);
            }

            current = hit_task;
            next = hit_task.next;
            interrupt = hit_task.interrupt;
        }
        else if (error_handling) {
            LogError << "error handling loop detected" << VAR(current.name);
            next.clear();
            interrupt.clear();
        }
        else {
            LogInfo << "handle error" << VAR(current.name);
            error_handling = true;
            next = current.on_error;
            interrupt.clear();
        }

        if (next.empty() && !task_stack.empty()) {
            LogInfo << "pop task_stack:" << current.name;
            current = context_->get_pipeline_data(task_stack.top());
            task_stack.pop();
        }
    }

    return !error_handling;
}

void PipelineTask::post_stop()
{
    need_to_stop_ = true;
}

std::pair<NodeDetail, /* is interrupt */ bool> PipelineTask::run_reco_and_action(
    const PipelineData::NextList& next,
    const PipelineData::NextList& interrupt,
    const PipelineData& pretask)
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
            LogError << "need_to_stop" << VAR(pretask.name);
            return {};
        }

        if (std::chrono::steady_clock::now() - start_clock > pretask.reco_timeout) {
            LogError << "Task timeout" << VAR(pretask.name) << VAR(pretask.reco_timeout);
            return {};
        }

        std::this_thread::sleep_until(current_clock + pretask.rate_limit);
    }

    auto node_detail = run_action(reco);

    return std::make_pair(node_detail, is_interrupt);
}

MAA_TASK_NS_END
