#include "ActionExecAgent.h"

#include <functional>

#include "MaaFramework/MaaAPI.h"
#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

ActionExecAgent::ActionExecAgent()
{
    custom_action_.run = &ActionExecAgent::maa_api_run;
    custom_action_.stop = &ActionExecAgent::maa_api_stop;
}

bool ActionExecAgent::register_for_maa_inst(MaaInstanceHandle handle, std::string_view name)
{
    return MaaRegisterCustomAction(handle, name.data(), &custom_action_, this);
}

bool ActionExecAgent::unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name)
{
    return MaaUnregisterCustomAction(handle, name.data());
}

bool ActionExecAgent::run(MaaSyncContextHandle sync_context, std::string_view task_name,
                          std::string_view custom_action_param, MaaRectHandle cur_box, std::string_view cur_rec_detail)
{
    return false;
}

void ActionExecAgent::stop() {}

MaaBool ActionExecAgent::maa_api_run(MaaSyncContextHandle sync_context, MaaStringView task_name,
                                     MaaStringView custom_action_param, MaaRectHandle cur_box,
                                     MaaStringView cur_rec_detail, MaaTransparentArg action_arg)
{
    auto* self = static_cast<ActionExecAgent*>(action_arg);
    if (!self) {
        LogError << "action_arg is nullptr";
        return false;
    }

    return self->run(sync_context, task_name, custom_action_param, cur_box, cur_rec_detail);
}

void ActionExecAgent::maa_api_stop(MaaTransparentArg action_arg)
{
    auto* self = static_cast<ActionExecAgent*>(action_arg);
    if (!self) {
        LogError << "action_arg is nullptr";
        return;
    }

    self->stop();
}

MAA_TOOLKIT_NS_END
