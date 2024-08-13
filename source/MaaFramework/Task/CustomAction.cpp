#include "CustomAction.h"

#include "Utils/NoWarningCV.hpp"

#include "Context.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

CustomAction::CustomAction(std::string name, MaaCustomActionCallback action, MaaTransparentArg trans_arg)
    : name_(std::move(name))
    , action_callback_(action)
    , trans_arg_(trans_arg)
{
}

bool CustomAction::run(
    const std::string& task_name,
    const MAA_RES_NS::Action::CustomParam& param,
    const cv::Rect& cur_box,
    const json::value& cur_rec_detail)
{
    LogFunc << VAR(task_name) << VAR(session_.name) << VAR_VOIDP(session_.action) << VAR_VOIDP(session_.trans_arg)
            << VAR(param.custom_param) << VAR(cur_box) << VAR(cur_rec_detail);

    if (!session_.action) {
        LogError << "Action is null" << VAR(task_name);
        return false;
    }

    std::string custom_param_string = param.custom_param.to_string();
    MaaRect box { .x = cur_box.x, .y = cur_box.y, .width = cur_box.width, .height = cur_box.height };
    std::string cur_rec_detail_string = cur_rec_detail.to_string();

    MaaBool ret = session_.action(
        &context_,
        task_name.c_str(),
        session_.name.c_str(),
        custom_param_string.c_str(),
        &box,
        cur_rec_detail_string.c_str(),
        session_.trans_arg);

    LogTrace << VAR_VOIDP(session_.action) << VAR(ret);

    return ret;
}

bool CustomAction::run(
    Context& context,
    const MAA_RES_NS::Action::CustomParam& param,
    const cv::Rect& cur_box,
    const json::value& cur_rec_detail)
{
    return false;
}

MAA_TASK_NS_END
