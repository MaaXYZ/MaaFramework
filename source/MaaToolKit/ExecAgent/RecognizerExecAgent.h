#pragma once

#include "Conf/Conf.h"
#include "ExecAgentBase.h"

#include "MaaFramework/Task/MaaCustomRecognizer.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class RecognizerExecAgent final : public ExecAgentBase, public SingletonHolder<RecognizerExecAgent>
{
public:
    RecognizerExecAgent();
    virtual ~RecognizerExecAgent() = default;

protected: // from ExecAgentBase
    virtual bool register_for_maa_inst(MaaInstanceHandle handle, std::string_view name) override;
    virtual bool unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name) override;

private:
    // for MaaCustomRecognizerAPI
    static MaaBool recognizer_analyze( //
        MaaSyncContextHandle sync_context, const MaaImageBufferHandle image, MaaStringView task_name,
        MaaStringView custom_recognition_param, MaaTransparentArg recognizer_arg,
        /*out*/ MaaRectHandle out_box,
        /*out*/ MaaStringBufferHandle out_detail);

private:
    MaaCustomRecognizerAPI custom_recognizer_;
};

MAA_TOOLKIT_NS_END
