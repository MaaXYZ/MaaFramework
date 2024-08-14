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
    PipelineData::NextList next_list = { PipelineData::NextObject { .name = entry_ } };

    while (!next_list.empty() && !need_to_stop_) {
        auto iter = run_reco_and_action(next_list);

        if (iter == next_list.cend()) {
            LogError << "Run task failed:" << next_list;
            return false;
        }
        const PipelineData::NextObject& hit_object = *iter;
        PipelineData hit_task = context_.get_pipeline_data(hit_object.name);

        switch (hit_object.then_goto) {
        case PipelineData::NextObject::ThenGotoLabel::None:
            if (hit_task.is_sub) { // for compatibility with v1.x
                const auto& ref = goto_stack.emplace(next_list);
                LogDebug << "push then_goto is_sub:" << hit_object.name << ref;
            }
            break;
        case PipelineData::NextObject::ThenGotoLabel::Head: {
            const auto& ref = goto_stack.emplace(next_list);
            LogDebug << "push then_goto head:" << hit_object.name << ref;
        } break;
        case PipelineData::NextObject::ThenGotoLabel::Current: {
            const auto& ref = goto_stack.emplace(iter, next_list.cend());
            LogDebug << "push then_goto current:" << hit_object.name << ref;
        } break;
        case PipelineData::NextObject::ThenGotoLabel::Following: {
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

void PipelineTask::post_stop()
{
    need_to_stop_ = true;
}

PipelineTask::NextIter PipelineTask::run_reco_and_action(const PipelineData::NextList& list)
{
    const NextIter NotFound = list.cend();

    if (!tasker_) {
        LogError << "tasker is null";
        return NotFound;
    }

    const auto timeout = GlobalOptionMgr::get_instance().pipeline_timeout();

    auto iter = list.cend();
    HitDetail hit_detail;

    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        iter = run_recogintion(list, hit_detail);
        if (iter != list.cend()) {
            // found
            break;
        }

        if (need_to_stop_) {
            LogError << "Task interrupted" << VAR(pre_hit_task_);
            return NotFound;
        }

        if (std::chrono::steady_clock::now() - start_time > timeout) {
            LogError << "Task timeout" << VAR(pre_hit_task_) << VAR(timeout);
            return NotFound;
        }
    }

    LogInfo << "Task hit:" << hit_detail.pipeline_data.name << VAR(hit_detail.reco_uid) << VAR(hit_detail.reco_hit)
            << VAR(hit_detail.reco_detail.to_string());

    tasker_->runtime_cache().set_pre_box(hit_detail.pipeline_data.name, hit_detail.reco_hit);

    bool run_ret = run_action(hit_detail);
    return run_ret ? iter : NotFound;
}

MAA_TASK_NS_END
