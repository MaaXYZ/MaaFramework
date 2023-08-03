#include "ConfigMgr.h"

#include <fstream>

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"

MAA_TOOLKIT_CONFIG_NS_BEGIN

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

MaaSize ConfigMgr::config_size() const
{
    return config_vec_.size();
}

MaaToolKitConfigHandle ConfigMgr::config_by_index(MaaSize index)
{
    if (index >= config_vec_.size()) {
        LogError << "Out of range" << VAR(index) << VAR(config_vec_.size());
        return nullptr;
    }
    return config_vec_[index].get();
}

MaaToolKitConfigHandle ConfigMgr::current()
{
    auto find_it = config_map_.find(current_);
    if (find_it == config_map_.end()) {
        LogError << "Current not found" << VAR(current_) << VAR(config_map_);
        return nullptr;
    }

    return find_it->second.get();
}

MaaToolKitConfigHandle ConfigMgr::add_config(MaaString config_name, MaaToolKitConfigHandle copy_from)
{
    LogInfo << VAR(config_name) << VAR_VOIDP(copy_from);

    if (config_map_.contains(config_name)) {
        LogError << "Config name already exists" << VAR(config_name) << VAR(config_map_);
        return nullptr;
    }

    Config new_config;
    if (auto cast = dynamic_cast<Config*>(copy_from); cast) {
        new_config = *cast;
    }
    new_config.set_name(config_name);

    auto new_config_ptr = std::make_shared<Config>(std::move(new_config));
    auto& ref = config_vec_.emplace_back(new_config_ptr);
    config_map_.emplace(config_name, new_config_ptr);

    LogTrace << VAR(config_name) << VAR(new_config_ptr) << VAR(*new_config_ptr) << VAR(config_vec_) << VAR(config_map_);

    return ref.get();
}

void ConfigMgr::del_task(MaaString config_name)
{
    LogInfo << VAR(config_name);

    bool removed = config_map_.erase(config_name) > 0;
    if (!removed) {
        LogError << "Config name not found in map" << VAR(config_name) << VAR(config_map_);
        return;
    }
    auto find_it =
        MAA_RNS::ranges::find_if(config_vec_, [&](const auto& config) { return config->get_name() == config_name; });
    if (find_it == config_vec_.end()) {
        LogError << "Config name not found in vec" << VAR(config_name) << VAR(config_vec_);
        return;
    }
    config_vec_.erase(find_it);

    LogTrace << VAR(config_name) << VAR(config_vec_) << VAR(config_map_);
}

bool ConfigMgr::set_current_config(MaaString config_name)
{
    LogInfo << VAR(config_name);

    auto find_it = config_map_.find(config_name);
    if (find_it == config_map_.end()) {
        LogError << "Config not found" << VAR(config_vec_) << VAR(config_map_);
        return false;
    }

    current_ = config_name;
    return true;
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
    if (current_.empty() || !config_map_.contains(current_)) {
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
    for (const auto& [key, config] : config_map_) {
        jconfig.emplace(key, config->to_json());
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
    LogFunc << VAR(name) << VAR(config);

    auto config_ptr = std::make_shared<Config>(std::move(config));

    auto map_it = config_map_.find(name);
    if (map_it == config_map_.end()) {
        auto vec_it =
            MaaRangesNS::ranges::find_if(config_vec_, [&](const auto& ptr) { return ptr->get_name() == name; });
        *vec_it = config_ptr;

        map_it->second = std::move(config_ptr);
    }
    else {
        config_vec_.emplace_back(config_ptr);
        config_map_.emplace(std::move(name), std::move(config_ptr));
    }
}

const std::filesystem::path& ConfigMgr::config_path() const
{
#ifdef _WIN32
    static const std::filesystem::path config_path("maa_toolkit.json");
    return config_path;
#else
    // TODO: unix 设备上应该不方便放当前目录，可能调整到 data 或者家目录之类的地方
    static const std::filesystem::path config_path("maa_toolkit.json");
    return config_path;
#endif
}

MAA_TOOLKIT_CONFIG_NS_END
