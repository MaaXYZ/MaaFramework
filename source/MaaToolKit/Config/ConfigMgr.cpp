#include "ConfigMgr.h"

#include <fstream>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"

#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Utils/Ranges.hpp"

MAA_TOOLKIT_CONFIG_NS_BEGIN

bool ConfigMgr::init()
{
    LogFunc << VAR(kConfigPath);

    if (!std::filesystem::exists(kConfigPath)) {
        generate_default_json();
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

size_t ConfigMgr::config_size() const
{
    return config_vec_.size();
}

MaaToolKitConfigHandle ConfigMgr::config_by_index(size_t index)
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

MaaToolKitConfigHandle ConfigMgr::add_config(std::string_view config_name, MaaToolKitConfigHandle copy_from)
{
    LogInfo << VAR(config_name) << VAR_VOIDP(copy_from);

    std::string str_config_name(config_name);
    if (config_map_.contains(str_config_name)) {
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
    config_map_.emplace(std::move(str_config_name), new_config_ptr);

    LogTrace << VAR(config_name) << VAR(new_config_ptr) << VAR(*new_config_ptr) << VAR(config_vec_) << VAR(config_map_);

    return ref.get();
}

bool ConfigMgr::del_config(std::string_view config_name)
{
    LogInfo << VAR(config_name);

    std::string str_config_name(config_name);
    bool removed = config_map_.erase(str_config_name) > 0;
    if (!removed) {
        LogError << "Config name not found in map" << VAR(config_name) << VAR(config_map_);
        return false;
    }
    auto find_it =
        MAA_RNS::ranges::find_if(config_vec_, [&](const auto& config) { return config->get_name() == config_name; });
    if (find_it == config_vec_.end()) {
        LogError << "Config name not found in vec" << VAR(config_name) << VAR(config_vec_);
        return false;
    }
    config_vec_.erase(find_it);

    LogTrace << VAR(config_name) << VAR(config_vec_) << VAR(config_map_);
    return true;
}

bool ConfigMgr::set_current_config(std::string_view config_name)
{
    LogInfo << VAR(config_name);

    std::string str_config_name(config_name);
    auto find_it = config_map_.find(str_config_name);
    if (find_it == config_map_.end()) {
        LogError << "Config not found" << VAR(config_vec_) << VAR(config_map_);
        return false;
    }

    current_ = config_name;
    return true;
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

    auto& config = json[kConfigKey];
    if (!parse_config(config)) {
        LogError << "Failed to parse config:" << VAR(config);
        return false;
    }

    auto& current = json[kCurrentKey];
    if (!parse_current(current)) {
        LogError << "Failed to parse current:" << VAR(current);
        return false;
    }

    return true;
}

bool ConfigMgr::parse_and_apply_policy(const json::value& policy_json)
{
    LogFunc << VAR(policy_json);

    bool logging = policy_json.get(kPolicyLoggging, kPolicyLogggingDefault);
    std::string logging_dir = path_to_utf8_string(logging ? kDebugDir : "");
    MaaSetGlobalOption(MaaGlobalOption_Logging, (void*)logging_dir.c_str(), logging_dir.size());

    bool debug_mode = policy_json.get(kPolicyDebugMode, kPolicyDebugModeDefault);
    MaaSetGlobalOption(MaaGlobalOption_DebugMode, &debug_mode, sizeof(debug_mode));

    return true;
}

bool ConfigMgr::parse_config(const json::value& config_json)
{
    LogFunc << VAR(config_json);

    if (!config_json.is_object()) {
        LogError << "Json is not an object:" << VAR(config_json);
        return false;
    }

    auto& config_obj = config_json.as_object();
    for (const auto& [key, jconfig] : config_obj) {
        Config config;
        if (!config.from_json(jconfig)) {
            LogError << "Failed to parse config:" << VAR(key) << jconfig;
            return false;
        }
        insert(key, std::move(config));
    }

    return true;
}

bool ConfigMgr::parse_current(const json::value& current_json)
{
    LogFunc << VAR(current_json);

    if (!current_json.is_string()) {
        LogError << "Json is not a string:" << VAR(current_json);
        return false;
    }

    current_ = current_json.as_string();
    return true;
}

bool ConfigMgr::generate_default_json() const
{
    LogInfo;

    json::value root;
    root[kPolicyKey] = default_policy();
    root[kConfigKey] = default_config();
    root[kCurrentKey] = kDefaultConfigName;

    return save(root);
}

json::value ConfigMgr::default_policy()
{
    return {
        { kPolicyLoggging, kPolicyLogggingDefault },
        { kPolicyDebugMode, kPolicyDebugModeDefault },
    };
}

json::value ConfigMgr::default_config()
{
    Config default_config;
    default_config.set_name(kDefaultConfigName);
    return json::array { default_config.to_json() };
}

bool ConfigMgr::dump() const
{
    LogFunc;

    return true;

    // TODO
    // json::object jconfig;
    // for (const auto& [key, config] : config_map_) {
    //    jconfig.emplace(key, config->to_json());
    //}

    // json::value root;
    // root[kConfigKey] = std::move(jconfig);
    // root[kCurrentKey] = current_;

    // return save(root);
}

bool ConfigMgr::save(const json::value& root) const
{
    LogFunc;

    std::filesystem::create_directories(kConfigPath.parent_path());
    std::ofstream ofs(kConfigPath, std::ios::out);
    if (!ofs.is_open()) {
        LogError << "Failed to open config file:" << kConfigPath;
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
    if (map_it != config_map_.end()) {
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

MAA_TOOLKIT_CONFIG_NS_END
