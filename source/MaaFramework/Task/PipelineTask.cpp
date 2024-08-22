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

PipelineTask::PipelineTask(std::string entry, Tasker* tasker, PipelineDataMap pp_override)
    : TaskBase(std::move(entry), tasker, std::move(pp_override))
{
}

bool PipelineTask::run()
{
    LogFunc << VAR(entry_);

    std::stack<PipelineData::NextList> goto_stack;
    PipelineData::NextList next = { entry_ };
    PipelineData::NextList catch_next;

    while (!next.empty() && !need_to_stop_) {
        auto [node_result, is_breakpoint] = run_reco_and_action(next, catch_next);

        if (!node_result.completed) {
            LogError << "Run task failed:" << next;
            return false;
        }

        PipelineData hit_task = context_.get_pipeline_data(node_result.name);
        if (is_breakpoint || hit_task.is_sub) { // for compatibility with v1.x
            const auto& ref = goto_stack.emplace(next);
            LogDebug << "push then_goto:" << hit_task.name << ref;
        }

        next = hit_task.next;
        catch_next = hit_task.catch_next;

        if (next.empty() && !goto_stack.empty()) {
            next = std::move(goto_stack.top());
            goto_stack.pop();
            LogDebug << "pop then_goto:" << next;
        }

        cur_task_ = hit_task.name;
    }
    return true;
}

void PipelineTask::post_stop()
{
    need_to_stop_ = true;
}

std::pair<NodeDetail, /* is breakpoint */ bool>
    PipelineTask::run_reco_and_action(const PipelineData::NextList& next, const PipelineData::NextList& catch_next)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }
    const auto timeout = GlobalOptionMgr::get_instance().pipeline_timeout();

    RecoResult reco;
    bool is_breakpoint = false;

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        cv::Mat image = screencap();
        reco = run_recogintion(image, next);
        if (reco.box) { // hit
            is_breakpoint = false;
            break;
        }

        reco = run_recogintion(image, catch_next);
        if (reco.box) { // hit
            is_breakpoint = true;
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

    return std::make_pair(node_detail, is_breakpoint);
}

MAA_TASK_NS_END
