#include "ConfigMgr.h"

#include <fstream>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"

#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/Ranges.hpp"

MAA_TOOLKIT_CONFIG_NS_BEGIN

bool ConfigMgr::init()
{
    LogFunc << VAR(kConfigPath);

    if (!std::filesystem::exists(kConfigPath)) {
        dump();
    }

    bool ret = load();
    if (!ret) {
        LogError << "Load failed";
        return false;
    }

    return ret;
}

bool ConfigMgr::uninit()
{
    LogFunc;

    return dump();
}

bool ConfigMgr::load()
{
    LogFunc << VAR(kConfigPath);

    auto json_opt = json::open(kConfigPath);
    if (!json_opt) {
        LogError << "Failed to open json file:" << kConfigPath;
        return false;
    }

    auto& json = *json_opt;
    if (!json.is_object()) {
        LogError << "Json is not an object:" << VAR(json);
        return false;
    }

    auto& policy = json[kPolicyKey];
    if (!parse_and_apply_policy(policy)) {
        LogError << "Failed to parse policy:" << VAR(policy);
        return false;
    }

    return true;
}

bool ConfigMgr::parse_and_apply_policy(const json::value& policy_json)
{
    LogFunc << VAR(policy_json);

    policy_logging_ = policy_json.get(kPolicyLoggging, policy_logging_);
    std::string logging_dir = policy_logging_ ? path_to_utf8_string(kDebugDir) : "";
    MaaSetGlobalOption(MaaGlobalOption_LogDir, (void*)logging_dir.c_str(), logging_dir.size());

    policy_save_draw_ = policy_json.get(kPolicySaveDraw, policy_save_draw_);
    MaaSetGlobalOption(MaaGlobalOption_SaveDraw, &policy_save_draw_, sizeof(policy_save_draw_));

    policy_recording_ = policy_json.get(kPolicyRecording, policy_recording_);
    MaaSetGlobalOption(MaaGlobalOption_Recording, &policy_recording_, sizeof(policy_recording_));

    policy_stdout_level_ = policy_json.get(kPolicyStdoutLevel, policy_stdout_level_);
    MaaSetGlobalOption(MaaGlobalOption_StdoutLevel, &policy_stdout_level_, sizeof(policy_stdout_level_));

    return true;
}

json::value ConfigMgr::to_json() const
{
    json::value root;
    root[kPolicyKey] = {
        { kPolicyLoggging, policy_logging_ },
        { kPolicySaveDraw, policy_save_draw_ },
        { kPolicyRecording, policy_recording_ },
        { kPolicyStdoutLevel, policy_stdout_level_ },
    };

    return root;
}

bool ConfigMgr::dump() const
{
    LogInfo;

    return save(to_json());
}

bool ConfigMgr::save(const json::value& root) const
{
    LogInfo;

    std::filesystem::create_directories(kConfigPath.parent_path());
    std::ofstream ofs(kConfigPath, std::ios::out);
    if (!ofs.is_open()) {
        LogError << "Failed to open config file:" << kConfigPath;
        return false;
    }
    ofs << root;
    return true;
}

MAA_TOOLKIT_CONFIG_NS_END
