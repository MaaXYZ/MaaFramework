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

    RecoResult fake_reco {
        .name = entry_,
        .box = box,
        .detail = reco_detail,
    };
    return run_action(fake_reco);
}

MAA_TASK_NS_END
