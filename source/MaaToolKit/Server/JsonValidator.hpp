#include "Conf/Conf.h"
#include <meojson/json.hpp>

MAA_TOOLKIT_SERVER_NS_BEGIN

inline std::optional<json::value> require_key(const json::object& obj, const std::string& key)
{
    auto val = obj.find(key);
    if (!val.has_value()) {
        return std::nullopt;
    }
    return val.value();
}

inline std::optional<std::string> require_key_as_string(const json::object& obj, const std::string& key)
{
    auto pv = require_key(obj, key);
    if (!pv.has_value()) {
        return std::nullopt;
    }
    auto& v = pv.value();
    if (!v.is_string()) {
        return std::nullopt;
    }
    return v.as_string();
}

inline std::optional<std::vector<std::string>> require_key_as_string_array(const json::object& obj,
                                                                           const std::string& key)
{
    auto pv = require_key(obj, key);
    if (!pv.has_value()) {
        return std::nullopt;
    }
    auto& v = pv.value();
    if (!v.is_array()) {
        return std::nullopt;
    }
    std::vector<std::string> res;
    for (const auto& va : v.as_array()) {
        if (!va.is_string()) {
            return std::nullopt;
        }
        res.push_back(va.as_string());
    }
    return res;
}

MAA_TOOLKIT_SERVER_NS_END
