#include "../include/forward.h"

#include "../include/info.h"
#include "../include/macro.h"
#include "../include/utils.h"
#include "../include/wrapper.h"

std::string version()
{
    return MaaVersion();
}

bool set_global_option_log_dir(std::string dir)
{
    return MaaSetGlobalOption(MaaGlobalOptionEnum::MaaGlobalOption_LogDir, dir.data(), dir.size());
}

bool set_global_option_save_draw(bool flag)
{
    return MaaSetGlobalOption(MaaGlobalOptionEnum::MaaGlobalOption_SaveDraw, &flag, sizeof(flag));
}

bool set_global_option_recording(bool flag)
{
    return MaaSetGlobalOption(MaaGlobalOptionEnum::MaaGlobalOption_Recording, &flag, sizeof(flag));
}

bool set_global_option_stdout_level(MaaLoggingLevel level)
{
    return MaaSetGlobalOption(MaaGlobalOptionEnum::MaaGlobalOption_StdoutLevel, &level, sizeof(level));
}

bool set_global_option_show_hit_draw(bool flag)
{
    return MaaSetGlobalOption(MaaGlobalOptionEnum::MaaGlobalOption_ShowHitDraw, &flag, sizeof(flag));
}

bool set_global_option_debug_mode(bool flag)
{
    return MaaSetGlobalOption(MaaGlobalOptionEnum::MaaGlobalOption_DebugMode, &flag, sizeof(flag));
}

void load_utility_utility(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(version);
    BIND(set_global_option_log_dir);
    BIND(set_global_option_save_draw);
    BIND(set_global_option_recording);
    BIND(set_global_option_stdout_level);
    BIND(set_global_option_show_hit_draw);
    BIND(set_global_option_debug_mode);
}
