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
        MaaTasker* handle,
        std::string_view name,
        ExecData& executor) override;
    virtual bool unregister_for_maa_inst(MaaTasker* handle, std::string_view name) override;

private:
    struct AnalyzeResult
    {
        cv::Rect box {};
        std::string detail;
    };

    std::optional<AnalyzeResult> analyze(
        ExecData& data,
        MaaContext* context,
        const cv::Mat& image,
        std::string_view task_name,
        std::string_view custom_recognition_param);

private:
    // for MaaCustomRecognizerAPI
    static MaaBool maa_api_analyze(
        MaaContext* context,
        const MaaImageBuffer* image,
        const char* task_name,
        const char* custom_recognition_param,
        MaaTransparentArg trans_arg,
        /*out*/ MaaRect* out_box,
        /*out*/ MaaStringBuffer* out_detail);

private:
    MaaCustomRecognizerCallback custom_recognizer_ {};
};

MAA_TOOLKIT_NS_END
