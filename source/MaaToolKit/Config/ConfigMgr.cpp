#include "ConfigMgr.h"

#include <fstream>

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"

MAA_TOOLKIT_NS_BEGIN

bool ConfigMgr::init()
{
    LogFunc << VAR(config_path());

    bool ret = false;
    if (std::filesystem::exists(config_path())) {
        ret = parse_json();
    }
    else {
        ret = create_default_config();
    }

    return ret;
}

bool ConfigMgr::uninit()
{
    LogFunc;

    return save();
}

bool ConfigMgr::parse_json()
{
    LogFunc << VAR(config_path());

    auto json_opt = json::open(config_path());
    if (!json_opt) {
        LogError << "Failed to open config file:" << config_path();
        return false;
    }

    auto& json = *json_opt;

    auto config_opt = json.find<json::object>(kConfigKey);
    if (!config_opt) {
        LogError << "Failed to find config key:" << kConfigKey;
        return false;
    }

    for (const auto& [key, jconfig] : *config_opt) {
        Config config;
        if (!config.from_json(jconfig)) {
            LogError << "Failed to parse config:" << VAR(key) << jconfig;
            return false;
        }
        insert(key, std::move(config));
    }

    current_ = json.get(kCurrentKey, std::string());
    if (current_.empty() || !configs_map_.contains(current_)) {
        LogError << "Failed to find current config:" << VAR(current_);
        return false;
    }

    return true;
}

bool ConfigMgr::create_default_config()
{
    Config config;
    // TODO: set default config

    static const std::string kDefaultKey = "Default";
    insert(kDefaultKey, std::move(config));

    return save();
}

bool ConfigMgr::save()
{
    LogFunc;

    json::object jconfig;
    for (const auto& [key, config] : configs_map_) {
        jconfig.emplace(key, config.to_json());
    }

    json::value root;
    root[kConfigKey] = std::move(jconfig);
    root[kCurrentKey] = current_;

    std::ofstream ofs(config_path(), std::ios::out);
    if (!ofs) {
        LogError << "Failed to open config file:" << config_path();
        return false;
    }
    ofs << root;
    return true;
}

void ConfigMgr::insert(std::string name, Config config)
{
    LogFunc << VAR(name);

    auto map_it = configs_map_.find(name);
    if (map_it != configs_map_.end()) {
        auto vec_it = MaaRangesNS::ranges::find_if(configs_vec_, [&](const auto& pair) { return pair.first == name; });
        vec_it->second = config;
        map_it->second = std::move(config);
    }
    else {
        configs_vec_.emplace_back(name, config);
        configs_map_.emplace(std::move(name), std::move(config));
    }
}

const std::filesystem::path& ConfigMgr::config_path() const
{
#ifdef _WIN32
    static const std::filesystem::path config_path("maa_toolkit.json");
    return config_path;
#else
// TODO
#endif
}

MAA_TOOLKIT_NS_END