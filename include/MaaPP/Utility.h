#pragma once

#include <string>
#include <string_view>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "./Exception.h"

namespace maapp
{

inline std::string_view version()
{
    return MaaVersion();
}

inline void set_log_dir(const std::string& dir)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_LogDir, const_cast<char*>(dir.c_str()), dir.length())) {
        throw FunctionFailed("MaaSetGlobalOption");
    }
}

inline void set_save_draw(bool enable)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_SaveDraw, &enable, sizeof(enable))) {
        throw FunctionFailed("MaaSetGlobalOption");
    }
}

inline void set_recording(bool enable)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_Recording, &enable, sizeof(enable))) {
        throw FunctionFailed("MaaSetGlobalOption");
    }
}

inline void set_stdout_level(MaaLoggingLevel level)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_StdoutLevel, &level, sizeof(level))) {
        throw FunctionFailed("MaaSetGlobalOption");
    }
}

inline void set_debug_mode(bool enable)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_DebugMode, &enable, sizeof(enable))) {
        throw FunctionFailed("MaaSetGlobalOption");
    }
}

}
