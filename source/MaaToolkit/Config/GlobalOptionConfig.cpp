#include "GlobalOptionConfig.h"

#include <fstream>
#include <ranges>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/Runtime.h"

MAA_TOOLKIT_NS_BEGIN

bool GlobalOptionConfig::init(
    const std::filesystem::path& user_path,
    const json::value& default_config)
{
    LogFunc << VAR(user_path);

    // FIXME: remove this
    if (auto old_config = user_path / kConfigPath_Deprecated; std::filesystem::exists(old_config)) {
        std::filesystem::remove(old_config);
    }

    config_path_ = user_path / kConfigPath;
    debug_dir_ = user_path / kDebugDir;

    if (!default_config.is<Option>()) {
        LogError << "failed to parse default_config" << VAR(default_config);
        return false;
    }

    if (!std::filesystem::exists(config_path_)) {
        option_ = (Option)default_config;
#ifdef MAA_DEBUG
        option_.stdout_level = MaaLoggingLevel_All;
#endif
        save();
    }

    bool ret = load();
    if (!ret) {
        LogError << "Load failed";
        return false;
    }

    return apply_option();
}

bool GlobalOptionConfig::load()
{
    LogFunc << VAR(config_path_);

    auto json_opt = json::open(config_path_);
    if (!json_opt) {
        LogError << "Failed to open json file" << config_path_;
        return false;
    }

    auto& json = *json_opt;

    if (!json.is<Option>()) {
        LogError << "Json is not an Option" << VAR(json);
        return false;
    }

    option_ = (Option)json;
    return true;
}

bool GlobalOptionConfig::apply_option()
{
    LogFunc << VAR(option_);

    MaaBool ret = true;

    std::string logging_dir = option_.logging ? path_to_utf8_string(debug_dir_) : "";
    ret &= MaaSetGlobalOption(
        MaaGlobalOption_LogDir,
        static_cast<void*>(logging_dir.data()),
        logging_dir.size());

    ret &=
        MaaSetGlobalOption(MaaGlobalOption_SaveDraw, &option_.save_draw, sizeof(option_.save_draw));

    ret &= MaaSetGlobalOption(
        MaaGlobalOption_Recording,
        &option_.recording,
        sizeof(option_.recording));

    ret &= MaaSetGlobalOption(
        MaaGlobalOption_StdoutLevel,
        &option_.stdout_level,
        sizeof(option_.stdout_level));

    ret &= MaaSetGlobalOption(
        MaaGlobalOption_ShowHitDraw,
        &option_.show_hit_draw,
        sizeof(option_.show_hit_draw));

    LogTrace << VAR(ret);
    return ret;
}

bool GlobalOptionConfig::save() const
{
    LogInfo;

    std::filesystem::create_directories(config_path_.parent_path());
    std::ofstream ofs(config_path_, std::ios::out);
    if (!ofs.is_open()) {
        LogError << "Failed to open config file" << config_path_;
        return false;
    }
    ofs << json::value(option_);
    return true;
}

MAA_TOOLKIT_NS_END