#include "CustomAction.h"

#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

CustomAction::CustomAction(std::string name, MAA_RES_NS::CustomActionSession session)
    : name_(std::move(name))
    , session_(session)
{
}

bool CustomAction::run(Context& context, const MAA_RES_NS::Action::CustomParam& param, const cv::Rect& box, const json::value& reco_detail)
{
    LogFunc << VAR(context.task_id()) << VAR(name_) << VAR_VOIDP(session_.action) << VAR_VOIDP(session_.trans_arg)
            << VAR(param.custom_param) << VAR(box) << VAR(reco_detail);

    if (!session_.action) {
        LogError << "Action is null" << VAR(name_);
        return false;
    }

    std::string custom_param_string = param.custom_param.to_string();
    MaaRect cbox { .x = box.x, .y = box.y, .width = box.width, .height = box.height };
    std::string reco_detail_string = reco_detail.to_string();

    MaaBool ret = session_.action(
        &context,
        context.task_id(),
        name_.c_str(),
        custom_param_string.c_str(),
        &cbox,
        reco_detail_string.c_str(),
        session_.trans_arg);

    LogTrace << VAR(name_) << VAR_VOIDP(session_.action) << VAR(session_.trans_arg) << VAR(ret);

    return ret;
}

MAA_TASK_NS_END
