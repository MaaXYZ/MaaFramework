#pragma once

#include "Conf/Conf.h"
#include "ExecAgentBase.h"

#include "MaaFramework/Task/MaaCustomAction.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class ActionExecAgent final
    : public ExecAgentBase
    , public SingletonHolder<ActionExecAgent>
{
public:
    ActionExecAgent();
    virtual ~ActionExecAgent() = default;

private: // from ExecAgentBase
    virtual bool register_for_maa_inst(
        MaaInstanceHandle handle,
        std::string_view name,
        ExecData& executor) override;
    virtual bool unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name) override;

private:
    bool
        run(ExecData& data,
            MaaSyncContextHandle sync_context,
            std::string_view task_name,
            std::string_view custom_action_param,
            MaaRectHandle cur_box,
            std::string_view cur_rec_detail);
    void stop(ExecData& data);

private:
    // for MaaCustomActionAPI
    static MaaBool maa_api_run(
        MaaSyncContextHandle sync_context,
        MaaStringView task_name,
        MaaStringView custom_action_param,
        MaaRectHandle cur_box,
        MaaStringView cur_rec_detail,
        MaaTransparentArg action_arg);
    static void maa_api_stop(MaaTransparentArg action_arg);

private:
    MaaCustomActionAPI custom_action_ {};
};

MAA_TOOLKIT_NS_END
