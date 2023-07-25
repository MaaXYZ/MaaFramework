#include "CustomAction.h"

#include "Utils/NoWarningCV.h"

#include "MaaCustomAction.h"
#include "Utils/Logger.hpp"

MAA_TASK_NS_BEGIN

CustomAction::CustomAction(MaaCustomActionHandle handle) : action_(handle) {}

bool CustomAction::run(const MAA_PIPELINE_RES_NS::Action::CustomParam& param, const cv::Rect& cur_box,
                       const json::value& rec_detail)
{
    LogFunc << VAR_VOIDP(action_) << VAR(param.custom_param) << VAR(cur_box) << VAR(rec_detail);

    if (!action_ || !action_->run) {
        LogError << "Action is null" << VAR_VOIDP(action_) << VAR_VOIDP(action_->run);
        return false;
    }

    MaaRect box { .x = cur_box.x, .y = cur_box.y, .width = cur_box.width, .height = cur_box.height };

    bool ret = action_->run(param.custom_param.to_string().c_str(), &box, rec_detail.to_string().c_str());
    LogTrace << VAR_VOIDP(action_) << VAR_VOIDP(action_->run) << VAR(ret);

    return ret;
}

MAA_TASK_NS_END
