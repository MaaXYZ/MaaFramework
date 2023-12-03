#include "RecognizerExecAgent.h"

#include <functional>

#include "MaaFramework/MaaAPI.h"
#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

RecognizerExecAgent::RecognizerExecAgent()
{
    custom_recognizer_.analyze = &RecognizerExecAgent::recognizer_analyze;
}

bool RecognizerExecAgent::register_for_maa_inst(MaaInstanceHandle handle, std::string_view name)
{
    return MaaRegisterCustomRecognizer(handle, name.data(), &custom_recognizer_, this);
}

bool RecognizerExecAgent::unregister_for_maa_inst(MaaInstanceHandle handle, std::string_view name)
{
    return MaaUnregisterCustomRecognizer(handle, name.data());
}

MaaBool RecognizerExecAgent::recognizer_analyze(MaaSyncContextHandle sync_context, const MaaImageBufferHandle image,
                                                MaaStringView task_name, MaaStringView custom_recognition_param,
                                                MaaTransparentArg recognizer_arg, MaaRectHandle out_box,
                                                MaaStringBufferHandle out_detail)
{
    return MaaBool();
}

MAA_TOOLKIT_NS_END
