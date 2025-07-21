#include "ActionTask.h"

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"
#include "Utils/JsonExt.hpp"
#include "Utils/Logger.h"
#include "Vision/VisionBase.h"

MAA_TASK_NS_BEGIN

bool ActionTask::run()
{
    LogFunc << VAR(entry_);

    return run_with_param({}, {}) != MaaInvalidId;
}

MaaNodeId ActionTask::run_with_param(const cv::Rect& box, const json::value& reco_detail)
{
    LogFunc << VAR(entry_);

    if (!tasker_) {
        LogError << "tasker is null";
        return MaaInvalidId;
    }

    RecoResult fake_reco {
        .reco_id = MAA_VISION_NS::VisionBase::generate_uid(),
        .name = entry_,
        .algorithm = "DirectHit",
        .box = box,
        .detail = reco_detail,
    };

    tasker_->runtime_cache().set_reco_detail(fake_reco.reco_id, fake_reco);

    std::chrono::milliseconds notify_cost; // 这玩意其实理论上应该能通过context自动添加到custom所属的cost里面? 不知道咋搞了
    return run_action(fake_reco, notify_cost).node_id;
}

MAA_TASK_NS_END
