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
    inline static const std::string kConfigKey = "Config";
    inline static const std::string kCurrentKey = "Current";

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

    bool parse_json();
    bool create_default_config();
    bool save();
    void insert(std::string name, Config config);

    const std::filesystem::path& config_path() const;

private:
    std::vector<std::shared_ptr<Config>> config_vec_; // for C API
    std::map<std::string, std::shared_ptr<Config>> config_map_;
    std::string current_;
};

MAA_TOOLKIT_CONFIG_NS_END
