#include "../include/loader.h"
#include "../include/wrapper.h"

#include <MaaFramework/MaaAPI.h>

std::string version()
{
    return MaaVersion();
}

bool set_global_option_log_dir(std::string dir)
{
    return MaaSetGlobalOption(MaaGlobalOption_LogDir, dir.data(), dir.size());
}

bool set_global_option_save_draw(bool flag)
{
    return MaaSetGlobalOption(MaaGlobalOption_SaveDraw, &flag, sizeof(flag));
}

bool set_global_option_recording(bool flag)
{
    return MaaSetGlobalOption(MaaGlobalOption_Recording, &flag, sizeof(flag));
}

bool set_global_option_stdout_level(MaaLoggingLevel level)
{
    return MaaSetGlobalOption(MaaGlobalOption_StdoutLevel, &level, sizeof(level));
}

bool set_global_option_show_hit_draw(bool flag)
{
    return MaaSetGlobalOption(MaaGlobalOption_ShowHitDraw, &flag, sizeof(flag));
}

bool set_global_option_debug_mode(bool flag)
{
    return MaaSetGlobalOption(MaaGlobalOption_DebugMode, &flag, sizeof(flag));
}

void load_utility_utility(
    Napi::Env env,
    Napi::Object& exports,
    Napi::External<ExtContextInfo> context)
{
    BIND(version);
    BIND(set_global_option_log_dir);
    BIND(set_global_option_save_draw);
    BIND(set_global_option_recording);
    BIND(set_global_option_stdout_level);
    BIND(set_global_option_show_hit_draw);
    BIND(set_global_option_debug_mode);
}
