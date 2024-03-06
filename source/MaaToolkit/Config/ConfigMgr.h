#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "ConfigAPI.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class ConfigMgr
    : public SingletonHolder<ConfigMgr>
    , public MaaToolkitConfigMgrAPI
{
    friend class SingletonHolder<ConfigMgr>;

public:
    inline static const std::filesystem::path kConfigPath = "config/maa_toolkit.json";
    inline static const std::filesystem::path kDebugDir = "debug";

public:
    inline static const std::string kPolicyKey = "policy";
    inline static const std::string kPolicyLoggging = "logging";
    inline static const std::string kPolicySaveDraw = "save_draw";
    inline static const std::string kPolicyRecording = "recording";
    inline static const std::string kPolicyStdoutLevel = "stdout_level";

public:
    virtual ~ConfigMgr() noexcept override = default;

public: // from MaaToolkitConfigMgrAPI
    virtual bool init() override;
    virtual bool uninit() override;

private:
    ConfigMgr() = default;

    bool load();
    bool parse_and_apply_policy(const json::value& policy_json);

    json::value to_json() const;
    bool dump() const;
    bool save(const json::value& root) const;

private:
    std::filesystem::path config_path_;
    std::filesystem::path debug_dir_;

private:
    bool policy_logging_ = true;
    bool policy_save_draw_ = false;
    bool policy_recording_ = false;

#ifdef MAA_DEBUG
    MaaLoggingLevel policy_stdout_level_ = MaaLoggingLevel_All;
#else
    MaaLoggingLevel policy_stdout_level_ = MaaLoggingLevel_Error;
#endif
};

MAA_TOOLKIT_NS_END
