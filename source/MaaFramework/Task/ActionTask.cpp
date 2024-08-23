#include "ActionTask.h"

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"
#include "Utils/JsonExt.hpp"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

bool ActionTask::run()
{
    LogFunc << VAR(entry_);

    return run_with_param({}, {}) != MaaInvalidId;
}

MaaNodeId ActionTask::run_with_param(const cv::Rect& box, const json::value& reco_detail)
{
    LogFunc << VAR(entry_);

    RecoResult fake_reco {
        .name = entry_,
        .box = box,
        .detail = reco_detail,
    };
    return run_action(fake_reco).node_id;
}

MAA_TASK_NS_END
