#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Conf/Conf.h"
#include "Config.h"
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
    inline static const std::string kConfigKey = "config";
    inline static const std::string kCurrentKey = "current";

    inline static const std::string kPolicyLoggging = "logging";
    inline static const std::string kPolicyDebugMode = "debug_mode";

    inline static const std::string kConfigDefaultName = "Default";

public:
    virtual ~ConfigMgr() noexcept override = default;

public: // from MaaToolKitConfigMgrAPI
    virtual bool init() override;
    virtual bool uninit() override;

    virtual size_t config_size() const override;
    virtual MaaToolKitConfigHandle config_by_index(size_t index) override;
    virtual MaaToolKitConfigHandle current() override;

    virtual MaaToolKitConfigHandle add_config(std::string_view config_name, MaaToolKitConfigHandle copy_from) override;
    virtual bool del_config(std::string_view config_name) override;
    virtual bool set_current_config(std::string_view config_name) override;

private:
    ConfigMgr() = default;

    bool load();
    bool parse_and_apply_policy(const json::value& policy_json);
    bool parse_config(const json::value& config_json);
    bool parse_current(const json::value& current_json);

    void generate_default_config();

    bool dump() const;
    bool save(const json::value& root) const;
    void insert(std::string name, Config config);

private:
    std::vector<std::shared_ptr<Config>> config_vec_; // for C API
    std::map<std::string, std::shared_ptr<Config>> config_map_;
    std::string current_ = kConfigDefaultName;

    bool policy_logging_ = true;
    bool policy_debug_mode_ = false;
};

MAA_TOOLKIT_CONFIG_NS_END
