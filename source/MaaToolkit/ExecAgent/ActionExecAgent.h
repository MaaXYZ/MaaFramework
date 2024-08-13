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
        MaaTasker* handle,
        std::string_view name,
        ExecData& executor) override;
    virtual bool unregister_for_maa_inst(MaaTasker* handle, std::string_view name) override;

private:
    bool
        run(ExecData& data,
            MaaContext* context,
            std::string_view task_name,
            std::string_view custom_action_param,
            MaaRect* cur_box,
            std::string_view cur_rec_detail);
    void stop(ExecData& data);

private:
    // for MaaCustomActionAPI
    static MaaBool maa_api_run(
        MaaContext* context,
        const char* task_name,
        const char* custom_action_param,
        MaaRect* cur_box,
        const char* cur_rec_detail,
        void* trans_arg);
    static void maa_api_stop(void* trans_arg);

private:
    MaaCustomActionCallback custom_action_ {};
};

MAA_TOOLKIT_NS_END
