#include "MaaToolKit/ExecAgent/MaaToolKitExecAgent.h"

#include <string_view>

#include "Utils/Logger.h"

MaaBool MaaToolKitRegisterCustomRecognizerExecutor( //
    MaaInstanceHandle handle, MaaStringView recognizer_name, MaaStringView recognizer_exec_path,
    MaaStringView recognizer_exec_param_json)
{
    return false;
}

MaaBool MaaToolKitRegisterCustomActionExecutor( //
    MaaInstanceHandle handle, MaaStringView action_name, MaaStringView action_exec_path,
    MaaStringView action_exec_param_json)
{
    return false;
}
