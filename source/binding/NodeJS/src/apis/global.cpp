#include "loader.h"

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "../foundation/spec.h"

std::string version_from_macro()
{
    return MAA_VERSION;
}

std::string version()
{
    return MaaVersion();
}

void set_log_dir(std::string value)
{
    if (!MaaGlobalSetOption(MaaGlobalOption_LogDir, value.data(), value.size())) {
        throw maajs::MaaError { "Global set log_dir failed" };
    }
}

void set_save_draw(bool value)
{
    if (!MaaGlobalSetOption(MaaGlobalOption_SaveDraw, &value, sizeof(value))) {
        throw maajs::MaaError { "Global set save_draw failed" };
    }
}

void set_save_on_error(bool value)
{
    if (!MaaGlobalSetOption(MaaGlobalOption_SaveOnError, &value, sizeof(value))) {
        throw maajs::MaaError { "Global set save_on_error failed" };
    }
}

void set_stdout_level(std::string level)
{
    int32_t value = 0;
    if (level == "Off") {
        value = MaaLoggingLevel_Off;
    }
    else if (level == "Fatal") {
        value = MaaLoggingLevel_Fatal;
    }
    else if (level == "Error") {
        value = MaaLoggingLevel_Error;
    }
    else if (level == "Warn") {
        value = MaaLoggingLevel_Warn;
    }
    else if (level == "Info") {
        value = MaaLoggingLevel_Info;
    }
    else if (level == "Debug") {
        value = MaaLoggingLevel_Debug;
    }
    else if (level == "Trace") {
        value = MaaLoggingLevel_Trace;
    }
    else if (level == "All") {
        value = MaaLoggingLevel_All;
    }
    else {
        throw maajs::MaaError { std::format("Global set stdout_level failed, invalid level {}", level) };
    }

    if (!MaaGlobalSetOption(MaaGlobalOption_StdoutLevel, &value, sizeof(value))) {
        throw maajs::MaaError { "Global set stdout_level failed" };
    }
}

void set_debug_mode(bool value)
{
    if (!MaaGlobalSetOption(MaaGlobalOption_DebugMode, &value, sizeof(value))) {
        throw maajs::MaaError { "Global set debug_mode failed" };
    }
}

void set_draw_quality(int value)
{
    if (!MaaGlobalSetOption(MaaGlobalOption_DrawQuality, &value, sizeof(value))) {
        throw maajs::MaaError { "Global set draw_quality failed" };
    }
}

void set_reco_image_cache_limit(size_t value)
{
    if (!MaaGlobalSetOption(MaaGlobalOption_RecoImageCacheLimit, &value, sizeof(value))) {
        throw maajs::MaaError { "Global set reco_image_cache_limit failed" };
    }
}

void config_init_option(std::string user_path, maajs::OptionalParam<std::string> default_json)
{
    if (!MaaToolkitConfigInitOption(user_path.c_str(), default_json.value_or("{}").c_str())) {
        throw maajs::MaaError { "Global config_init_option failed" };
    }
}

maajs::ObjectType load_global(maajs::EnvType env)
{
    auto globalObject = maajs::ObjectType::New(env);

    MAA_BIND_GETTER(globalObject, "version_from_macro", version_from_macro);
    MAA_BIND_GETTER(globalObject, "version", version);
    MAA_BIND_SETTER(globalObject, "log_dir", set_log_dir);
    MAA_BIND_SETTER(globalObject, "save_draw", set_save_draw);
    MAA_BIND_SETTER(globalObject, "save_on_error", set_save_on_error);
    MAA_BIND_SETTER(globalObject, "stdout_level", set_stdout_level);
    MAA_BIND_SETTER(globalObject, "debug_mode", set_debug_mode);
    MAA_BIND_SETTER(globalObject, "draw_quality", set_draw_quality);
    MAA_BIND_SETTER(globalObject, "reco_image_cache_limit", set_reco_image_cache_limit);
    MAA_BIND_FUNC(globalObject, "config_init_option", config_init_option);

    return globalObject;
}
