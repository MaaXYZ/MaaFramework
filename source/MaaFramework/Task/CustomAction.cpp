#include "CustomAction.h"

#include "Utils/NoWarningCV.hpp"

#include "MaaFramework/Task/MaaCustomAction.h"
#include "SyncContext.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

CustomAction::CustomAction(MaaCustomActionHandle handle, MaaTransparentArg handle_arg_, InstanceInternalAPI* inst)
    : action_(handle), action_arg_(handle_arg_), inst_(inst)
{}

bool CustomAction::run(const std::string& task_name, const MAA_RES_NS::Action::CustomParam& param,
                       const cv::Rect& cur_box, const json::value& cur_rec_detail)
{
    LogFunc << VAR(task_name) << VAR_VOIDP(action_) << VAR(param.custom_param) << VAR(cur_box);

    if (!action_ || !action_->run) {
        LogError << "Action is null" << VAR_VOIDP(action_) << VAR_VOIDP(action_->run);
        return false;
    }

    SyncContext sync_ctx(inst_);
    std::string custom_param_string = param.custom_param.to_string();
    MaaRect box { .x = cur_box.x, .y = cur_box.y, .width = cur_box.width, .height = cur_box.height };
    std::string cur_rec_detail_string = cur_rec_detail.to_string();

    bool ret = action_->run(&sync_ctx, task_name.c_str(), custom_param_string.c_str(), &box,
                            cur_rec_detail_string.c_str(), action_arg_);
    LogDebug << VAR_VOIDP(action_) << VAR_VOIDP(action_->run) << VAR(ret);

    return ret;
}

MAA_TASK_NS_END
