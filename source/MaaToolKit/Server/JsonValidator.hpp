#include "Conf/Conf.h"
#include <meojson/json.hpp>

MAA_TOOLKIT_NS_BEGIN

struct JsonValidateFailedException : public std::exception
{
    JsonValidateFailedException(std::string&& s) : err(s) {}

    const char* what() const override { return err.c_str(); }

    std::string err;
};

inline json::value require_key(const json::object& obj, const std::string& key)
{
    auto val = obj.find(key);
    if (!val.has_value()) {
        throw JsonValidateFailedException("require key " + key);
    }
    return val.value();
}

inline std::string require_key_as_string(const json::object& obj, const std::string& key)
{
    auto v = require_key(obj, key);
    if (!v.is_string()) {
        throw JsonValidateFailedException("key " + key + " must be string");
    }
    return v.as_string();
}

inline std::vector<std::string> require_key_as_string_array(const json::object& obj, const std::string& key)
{
    auto v = require_key(obj, key);
    if (!v.is_array()) {
        throw JsonValidateFailedException("key " + key + " must be string array");
    }
    std::vector<std::string> res;
    for (const auto& va : v.as_array()) {
        if (!va.is_string()) {
            throw JsonValidateFailedException("key " + key + " must be string array");
        }
        res.push_back(va.as_string());
    }
    return res;
}

MAA_TOOLKIT_NS_END
