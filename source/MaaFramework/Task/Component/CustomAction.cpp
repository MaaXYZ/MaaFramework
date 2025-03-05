#include "CustomAction.h"

#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

bool CustomAction::run(
    Context& context,
    std::string node_name,
    MAA_RES_NS::CustomActionSession session,
    const MAA_RES_NS::Action::CustomParam& param,
    MaaRecoId reco_id,
    const cv::Rect& rect)
{
    LogFunc << VAR(context.task_id()) << VAR(node_name) << VAR_VOIDP(session.action) << VAR_VOIDP(session.trans_arg)
            << VAR(param.name) << VAR(param.custom_param) << VAR(reco_id) << VAR(rect);

    if (!session.action) {
        LogError << "Action is null" << VAR(node_name) << VAR(param.name);
        return false;
    }

    std::string custom_param_string = param.custom_param.to_string();
    MaaRect crect { .x = rect.x, .y = rect.y, .width = rect.width, .height = rect.height };

    bool ret = session.action(
        &context,
        context.task_id(),
        node_name.c_str(),
        param.name.c_str(),
        custom_param_string.c_str(),
        reco_id,
        &crect,
        session.trans_arg);

    LogDebug << VAR(node_name) << VAR(param.name) << VAR_VOIDP(session.action) << VAR(session.trans_arg) << VAR(ret);

    return ret;
}

MAA_TASK_NS_END
