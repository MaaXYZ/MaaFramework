#include "CustomAction.h"

#include "Utils/NoWarningCV.hpp"

#include "MaaFramework/Task/MaaCustomAction.h"
#include "SyncContext.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

CustomAction::CustomAction(CustomActionSession session, InstanceInternalAPI* inst)
    : session_(std::move(session))
    , inst_(inst)
{
}

bool CustomAction::run(
    const std::string& task_name,
    const MAA_RES_NS::Action::CustomParam& param,
    const cv::Rect& cur_box,
    const json::value& cur_rec_detail)
{
    LogFunc << VAR(task_name) << VAR_VOIDP(session_.action) << VAR(param.custom_param)
            << VAR(cur_box);

    if (!session_.action || !session_.action->run) {
        LogError << "Action is null" << VAR(task_name);
        return false;
    }

    SyncContext sync_ctx(inst_);
    std::string custom_param_string = param.custom_param.to_string();
    MaaRect box { .x = cur_box.x,
                  .y = cur_box.y,
                  .width = cur_box.width,
                  .height = cur_box.height };
    std::string cur_rec_detail_string = cur_rec_detail.to_string();

    bool ret = session_.action->run(
        &sync_ctx,
        task_name.c_str(),
        custom_param_string.c_str(),
        &box,
        cur_rec_detail_string.c_str(),
        session_.action_arg);
    LogTrace << VAR_VOIDP(session_.action) << VAR_VOIDP(session_.action->run) << VAR(ret);

    return ret;
}

MAA_TASK_NS_END