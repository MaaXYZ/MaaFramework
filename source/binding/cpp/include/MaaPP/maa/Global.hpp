// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <string_view>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>
#include <meojson/json.hpp>

#include "MaaPP/maa/Exception.hpp"

namespace maa
{

inline auto version()
{
    return MaaVersion();
}

inline void init(const std::string& user_path, const json::object& default_json = {})
{
    if (!MaaToolkitInitOptionConfig(user_path.c_str(), default_json.to_string().c_str())) {
        throw FunctionFailed("MaaToolkitInitOptionConfig");
    }
}

inline void set_log_dir(std::string_view dir)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_LogDir, const_cast<char*>(dir.data()), dir.size())) {
        throw FunctionFailed("MaaSetGlobalOption");
    }
}

inline void set_save_draw(bool enable)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_SaveDraw, &enable, sizeof(enable))) {
        throw FunctionFailed("MaaToolkitInitOptionConfig");
    }
}

inline void set_recording(bool enable)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_Recording, &enable, sizeof(enable))) {
        throw FunctionFailed("MaaToolkitInitOptionConfig");
    }
}

inline void set_stdout_level(MaaLoggingLevelEunm level)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_StdoutLevel, &level, sizeof(level))) {
        throw FunctionFailed("MaaToolkitInitOptionConfig");
    }
}

inline void set_show_hit_draw(bool enable)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_ShowHitDraw, &enable, sizeof(enable))) {
        throw FunctionFailed("MaaToolkitInitOptionConfig");
    }
}

inline void set_debug_message(bool enable)
{
    if (!MaaSetGlobalOption(MaaGlobalOption_DebugMessage, &enable, sizeof(enable))) {
        throw FunctionFailed("MaaToolkitInitOptionConfig");
    }
}

}
