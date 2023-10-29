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

MAA_TOOLKIT_CONFIG_NS_BEGIN

class ConfigMgr : public SingletonHolder<ConfigMgr>, public MaaToolKitConfigMgrAPI
{
    friend class SingletonHolder<ConfigMgr>;

public:
    // TODO: linux 可能要区分下放到家目录之类的
    inline static const std::filesystem::path kUserDir = ".";
    inline static const std::filesystem::path kConfigPath = kUserDir / "config" / "maa_toolkit.json";
    inline static const std::filesystem::path kDebugDir = kUserDir / "debug";

public:
    inline static const std::string kPolicyKey = "policy";
    inline static const std::string kPolicyLoggging = "logging";
    inline static const std::string kPolicyDebugMode = "debug_mode";
    inline static const std::string kPolicyRecording = "recording";

public:
    virtual ~ConfigMgr() noexcept override = default;

public: // from MaaToolKitConfigMgrAPI
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
    bool policy_logging_ = true;
    bool policy_debug_mode_ = false;
    bool policy_recording_ = false;
};

MAA_TOOLKIT_CONFIG_NS_END
