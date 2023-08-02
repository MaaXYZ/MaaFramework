#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Conf/Conf.h"
#include "Config.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_CONFIG_NS_BEGIN

class ConfigMgr : public SingletonHolder<ConfigMgr>
{
    friend class SingletonHolder<ConfigMgr>;

public:
    inline static const std::string kConfigKey = "Config";
    inline static const std::string kCurrentKey = "Current";

public:
    virtual ~ConfigMgr() noexcept override = default;

public:
    bool init();
    bool uninit();

private:
    ConfigMgr();

    bool parse_json();
    bool create_default_config();
    bool save();
    void insert(std::string name, Config config);

    const std::filesystem::path& config_path() const;

private:
    std::vector<Config> configs_vec_; // for C API
    std::string current_;
};

MAA_TOOLKIT_CONFIG_NS_END
