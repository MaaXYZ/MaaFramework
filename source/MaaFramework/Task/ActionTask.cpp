#include "ActionTask.h"

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"
#include "Utils/JsonExt.hpp"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

ActionTask::ActionTask(std::string entry, Tasker* tasker, PipelineDataMap pp_override)
    : TaskBase(std::move(entry), tasker, std::move(pp_override))
{
}

bool ActionTask::run()
{
    LogFunc << VAR(entry_);

    return run_with_param({}, {});
}

bool ActionTask::run_with_param(const cv::Rect& box, const json::value& reco_detail)
{
    LogFunc << VAR(entry_);

    HitDetail fake_hit { .reco_hit = box, .reco_detail = reco_detail, .pipeline_data = context_.get_pipeline_data(entry_) };
    return run_action(fake_hit);
}

MAA_TASK_NS_END
