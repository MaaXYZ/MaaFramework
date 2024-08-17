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
    PipelineData::NextList next_list = { entry_ };

    while (!next_list.empty() && !need_to_stop_) {
        auto node_result = run_reco_and_action(next_list);

        if (!node_result.action_completed) {
            LogError << "Run task failed:" << next_list;
            return false;
        }

        PipelineData hit_task = context_.get_pipeline_data(node_result.name);
        if (hit_task.is_sub) { // for compatibility with v1.x
            const auto& ref = goto_stack.emplace(next_list);
            LogDebug << "push then_goto is_sub:" << hit_task.name << ref;
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

void PipelineTask::post_stop()
{
    need_to_stop_ = true;
}

NodeDetail PipelineTask::run_reco_and_action(const PipelineData::NextList& list)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }
    const auto timeout = GlobalOptionMgr::get_instance().pipeline_timeout();

    RecoResult reco;

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        reco = run_recogintion(screencap(), list);
        if (reco.box) {
            // hit
            break;
        }

        if (need_to_stop_) {
            LogError << "Task interrupted" << VAR(pre_hit_task_);
            return {};
        }

        if (std::chrono::steady_clock::now() - start_time > timeout) {
            LogError << "Task timeout" << VAR(pre_hit_task_) << VAR(timeout);
            return {};
        }
    }

    if (!reco.box) {
        return {};
    }

    LogInfo << "Task hit" << VAR(reco.name) << VAR(reco.box);

    tasker_->runtime_cache().set_pre_box(reco.name, *reco.box);

    bool run_ret = run_action(reco);
    if (!run_ret) {
        LogError << "Run action failed" << VAR(reco.name);
        return {};
    }

    return { reco.name, reco.uid, false };
}

MAA_TASK_NS_END
