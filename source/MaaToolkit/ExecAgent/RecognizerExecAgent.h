#pragma once

#include <functional>
#include <map>

#include "Conf/Conf.h"
#include "ExecAgentBase.h"
#include "MaaFramework/Task/MaaCustomRecognizer.h"
#include "Utils/NoWarningCVMat.hpp"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class RecognizerExecAgent final
    : public ExecAgentBase
    , public SingletonHolder<RecognizerExecAgent>
{
public:
    RecognizerExecAgent();
    virtual ~RecognizerExecAgent() = default;

protected: // from ExecAgentBase
    virtual bool register_for_maa_inst(
        MaaInstanceHandle handle,
        std::string_view name,
        ExecData& executor) override;
    virtual bool unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name) override;

private:
    struct AnalyzeResult
    {
        cv::Rect box {};
        std::string detail;
    };

    std::optional<AnalyzeResult> analyze(
        ExecData& data,
        MaaSyncContextHandle sync_context,
        const cv::Mat& image,
        std::string_view task_name,
        std::string_view custom_recognition_param);

private:
    // for MaaCustomRecognizerAPI
    static MaaBool maa_api_analyze(
        MaaSyncContextHandle sync_context,
        const MaaImageBufferHandle image,
        MaaStringView task_name,
        MaaStringView custom_recognition_param,
        MaaTransparentArg recognizer_arg,
        /*out*/ MaaRectHandle out_box,
        /*out*/ MaaStringBufferHandle out_detail);

private:
    MaaCustomRecognizerAPI custom_recognizer_ {};
};

MAA_TOOLKIT_NS_END
