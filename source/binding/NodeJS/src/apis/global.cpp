#include "loader.h"
#include "resource.h"

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

void config_init_option(std::string user_path, maajs::OptionalParam<std::string> default_json)
{
    if (!MaaToolkitConfigInitOption(user_path.c_str(), default_json.value_or("{}").c_str())) {
        throw maajs::MaaError { "Global config_init_option failed" };
    }
}

void pi_load(std::string interface_path)
{
    if (!MaaToolkitProjectInterfaceLoad(interface_path.c_str())) {
        throw maajs::MaaError { "ProjectInterface load failed" };
    }
}

bool pi_loaded()
{
    return MaaToolkitProjectInterfaceLoaded();
}

void pi_bind_resource(maajs::ValueType resource)
{
    auto* impl = maajs::native_class_unwrap<ResourceImpl>(resource);
    if (!impl || !impl->resource) {
        throw maajs::MaaError { "ProjectInterface bind_resource failed: invalid resource" };
    }
    if (!MaaToolkitProjectInterfaceBindResource(impl->resource)) {
        throw maajs::MaaError { "ProjectInterface bind_resource failed" };
    }
}

void pi_start_agent()
{
    if (!MaaToolkitProjectInterfaceStartAgent()) {
        throw maajs::MaaError { "ProjectInterface start_agent failed" };
    }
}

void pi_stop_agent()
{
    MaaToolkitProjectInterfaceStopAgent();
}

bool pi_agent_running()
{
    return MaaToolkitProjectInterfaceAgentRunning();
}

bool pi_agent_connected()
{
    return MaaToolkitProjectInterfaceAgentConnected();
}

maajs::ObjectType load_global(maajs::EnvType env)
{
    auto globalObject = maajs::ObjectType::New(env);

    MAA_BIND_GETTER(globalObject, "version_from_macro", version_from_macro);
    MAA_BIND_GETTER(globalObject, "version", version);
    MAA_BIND_SETTER(globalObject, "log_dir", set_log_dir);
    MAA_BIND_SETTER(globalObject, "save_draw", set_save_draw);
    MAA_BIND_SETTER(globalObject, "stdout_level", set_stdout_level);
    MAA_BIND_SETTER(globalObject, "debug_mode", set_debug_mode);
    MAA_BIND_FUNC(globalObject, "config_init_option", config_init_option);
    MAA_BIND_FUNC(globalObject, "pi_load", pi_load);
    MAA_BIND_GETTER(globalObject, "pi_loaded", pi_loaded);
    MAA_BIND_FUNC(globalObject, "pi_bind_resource", pi_bind_resource);
    MAA_BIND_FUNC(globalObject, "pi_start_agent", pi_start_agent);
    MAA_BIND_FUNC(globalObject, "pi_stop_agent", pi_stop_agent);
    MAA_BIND_GETTER(globalObject, "pi_agent_running", pi_agent_running);
    MAA_BIND_GETTER(globalObject, "pi_agent_connected", pi_agent_connected);

    return globalObject;
}
