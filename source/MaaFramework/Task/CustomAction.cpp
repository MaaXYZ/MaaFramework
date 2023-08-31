#include "CustomAction.h"

#include "Utils/NoWarningCV.hpp"

#include "MaaFramework/MaaCustomAction.h"
#include "SyncContext.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

CustomAction::CustomAction(MaaCustomActionHandle handle, InstanceInternalAPI* inst) : action_(handle), inst_(inst) {}

bool CustomAction::run(const std::string& task_name, const MAA_PIPELINE_RES_NS::Action::CustomParam& param,
                       const cv::Rect& cur_box, const std::string& cur_rec_detail)
{
    LogFunc << VAR(task_name) << VAR_VOIDP(action_) << VAR(param.custom_param) << VAR(cur_box);

    if (!action_ || !action_->run) {
        LogError << "Action is null" << VAR_VOIDP(action_) << VAR_VOIDP(action_->run);
        return false;
    }

    SyncContext sync_ctx(inst_);
    MaaRect box { .x = cur_box.x, .y = cur_box.y, .width = cur_box.width, .height = cur_box.height };

    bool ret = action_->run(&sync_ctx, task_name.c_str(), param.custom_param.to_string().c_str(), &box, cur_rec_detail.c_str());
    LogDebug << VAR_VOIDP(action_) << VAR_VOIDP(action_->run) << VAR(ret);

    return ret;
}

MAA_TASK_NS_END
