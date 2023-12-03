#include "ActionExecAgent.h"

#include <functional>

#include "MaaFramework/MaaAPI.h"
#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

ActionExecAgent::ActionExecAgent()
{
    custom_action_.run = &ActionExecAgent::action_run;
    custom_action_.stop = &ActionExecAgent::action_stop;
}

bool ActionExecAgent::register_for_maa_inst(MaaInstanceHandle handle, std::string_view name)
{
    return MaaRegisterCustomAction(handle, name.data(), &custom_action_, this);
}

bool ActionExecAgent::unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name)
{
    return MaaUnregisterCustomAction(handle, name.data());
}

MaaBool ActionExecAgent::action_run(MaaSyncContextHandle sync_context, MaaStringView task_name,
                                    MaaStringView custom_action_param, MaaRectHandle cur_box,
                                    MaaStringView cur_rec_detail, MaaTransparentArg action_arg)
{
    return MaaBool();
}

void ActionExecAgent::action_stop(MaaTransparentArg action_arg) {}

MAA_TOOLKIT_NS_END
