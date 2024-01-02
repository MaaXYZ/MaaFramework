#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaBool MAA_TOOLKIT_API MaaToolkitRegisterCustomRecognizerExecutor( //
        MaaInstanceHandle handle, MaaStringView recognizer_name, MaaStringView recognizer_exec_path,
        MaaStringView recognizer_exec_param_json);
    MaaBool MAA_TOOLKIT_API MaaToolkitUnregisterCustomRecognizerExecutor( //
        MaaInstanceHandle handle, MaaStringView recognizer_name);

    MaaBool MAA_TOOLKIT_API MaaToolkitRegisterCustomActionExecutor( //
        MaaInstanceHandle handle, MaaStringView action_name, MaaStringView action_exec_path,
        MaaStringView action_exec_param_json);
    MaaBool MAA_TOOLKIT_API MaaToolkitUnregisterCustomActionExecutor( //
        MaaInstanceHandle handle, MaaStringView action_name);

#ifdef __cplusplus
}
#endif
