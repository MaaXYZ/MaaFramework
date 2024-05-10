// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include "MaaPP/module/Module.h"

#ifndef MAAPP_USE_MODULE

#include <string_view>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>
#include <meojson/json.hpp>

#endif

namespace maa
{

MAAPP_EXPORT inline auto version()
{
    return MaaVersion();
}

MAAPP_EXPORT inline bool init(const std::string& user_path, const json::object& default_json = {})
{
    return MaaToolkitInitOptionConfig(user_path.c_str(), default_json.to_string().c_str());
}

MAAPP_EXPORT inline bool set_log_dir(std::string_view dir)
{
    return MaaSetGlobalOption(MaaGlobalOption_LogDir, const_cast<char*>(dir.data()), dir.size());
}

MAAPP_EXPORT inline bool set_save_draw(bool enable)
{
    return MaaSetGlobalOption(MaaGlobalOption_SaveDraw, &enable, sizeof(enable));
}

MAAPP_EXPORT inline bool set_recording(bool enable)
{
    return MaaSetGlobalOption(MaaGlobalOption_Recording, &enable, sizeof(enable));
}

MAAPP_EXPORT inline bool set_stdout_level(MaaLoggingLevelEunm level)
{
    return MaaSetGlobalOption(MaaGlobalOption_StdoutLevel, &level, sizeof(level));
}

MAAPP_EXPORT inline bool set_show_hit_draw(bool enable)
{
    return MaaSetGlobalOption(MaaGlobalOption_ShowHitDraw, &enable, sizeof(enable));
}

MAAPP_EXPORT inline bool set_debug_message(bool enable)
{
    return MaaSetGlobalOption(MaaGlobalOption_DebugMessage, &enable, sizeof(enable));
}

}
