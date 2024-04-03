#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_TOOLKIT_API MaaBool MaaToolkitRegisterCustomRecognizerExecutor(
        MaaInstanceHandle handle,
        MaaStringView recognizer_name,
        MaaStringView recognizer_exec_path,
        MaaStringView recognizer_exec_param_json);
    MAA_TOOLKIT_API MaaBool MaaToolkitUnregisterCustomRecognizerExecutor(
        MaaInstanceHandle handle,
        MaaStringView recognizer_name);

    MAA_TOOLKIT_API MaaBool MaaToolkitRegisterCustomActionExecutor(
        MaaInstanceHandle handle,
        MaaStringView action_name,
        MaaStringView action_exec_path,
        MaaStringView action_exec_param_json);
    MAA_TOOLKIT_API MaaBool MaaToolkitUnregisterCustomActionExecutor(
        MaaInstanceHandle handle,
        MaaStringView action_name);

#ifdef __cplusplus
}
#endif
