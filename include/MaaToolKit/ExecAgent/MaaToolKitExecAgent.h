#pragma once

#include "../MaaToolKitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaBool MAA_TOOLKIT_API MaaToolKitRegisterCustomRecognizerExecutor( //
        MaaInstanceHandle handle, MaaStringView recognizer_name, MaaStringView recognizer_exec_path,
        MaaStringView recognizer_exec_param_json, MaaToolKitExecAgentArgvTransferMode argv_mode);
    MaaBool MAA_TOOLKIT_API MaaToolKitUnregisterCustomRecognizerExecutor( //
        MaaInstanceHandle handle, MaaStringView recognizer_name);

    MaaBool MAA_TOOLKIT_API MaaToolKitRegisterCustomActionExecutor( //
        MaaInstanceHandle handle, MaaStringView action_name, MaaStringView action_exec_path,
        MaaStringView action_exec_param_json, MaaToolKitExecAgentArgvTransferMode argv_mode);
    MaaBool MAA_TOOLKIT_API MaaToolKitUnregisterCustomActionExecutor( //
        MaaInstanceHandle handle, MaaStringView action_name);

#ifdef __cplusplus
}
#endif
