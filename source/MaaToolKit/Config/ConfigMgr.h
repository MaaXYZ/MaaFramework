#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Base/SingletonHolder.hpp"
#include "Common/MaaConf.h"
#include "Config.h"

MAA_TOOLKIT_NS_BEGIN

class ConfigMgr : public SingletonHolder<ConfigMgr>
{
    friend class SingletonHolder<ConfigMgr>;

public:
    inline static const std::string kConfigKey = "Configuration";
    inline static const std::string kCurrentKey = "Current";

public:
    virtual ~ConfigMgr() noexcept override = default;

public:
    bool init();
    bool uninit();

private:
    ConfigMgr() = default;

    bool parse_json();
    bool create_default_config();
    bool save();
    void insert(std::string name, Config config);

    const std::filesystem::path& config_path() const;

private:
    std::vector<std::pair<std::string, Config>> configs_vec_; // for C API
    std::map<std::string, Config> configs_map_;
    std::string current_;
};

MAA_TOOLKIT_NS_END