#pragma once

#include "Conf/Conf.h"
#include "ExecAgentBase.h"

#include "MaaFramework/Task/MaaCustomAction.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class ActionExecAgent final : public ExecAgentBase, public SingletonHolder<ActionExecAgent>
{
public:
    ActionExecAgent();
    virtual ~ActionExecAgent() = default;

private: // from ExecAgentBase
    virtual bool register_for_maa_inst(MaaInstanceHandle handle, std::string_view name) override;
    virtual bool unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name) override;

private: // for MaaCustomActionAPI
    static MaaBool action_run(MaaSyncContextHandle sync_context, MaaStringView task_name,
                              MaaStringView custom_action_param, MaaRectHandle cur_box, MaaStringView cur_rec_detail,
                              MaaTransparentArg action_arg);
    static void action_stop(MaaTransparentArg action_arg);

private:
    MaaCustomActionAPI custom_action_;
};

MAA_TOOLKIT_NS_END
