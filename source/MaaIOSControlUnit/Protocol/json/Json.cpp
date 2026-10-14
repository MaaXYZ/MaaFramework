#include "json/Json.h"
#include "i18n/Translation.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace maa::ios::json
{

std::optional<Value> parse(std::string_view text, std::string* err)
{
    if (err) {
        err->clear();
    }
    if (text.size() > (4u << 20)) {
        if (err) {
            *err = MAA_IOS_TR("Input too large");
        }
        return std::nullopt;
    }
    try {
        auto bounded = [](int depth, Value::parse_event_t, Value&) {
            // 抛出异常立即终止解析，不能只过滤超深的节点后继续消费输入。
            if (depth > 64) {
                throw std::runtime_error(MAA_IOS_TR("Nesting too deep"));
            }
            return true;
        };
        return Value::parse(text.begin(), text.end(), bounded);
    }
    catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
        return std::nullopt;
    }
}

std::string write(const Value& value)
{
    return value.dump();
}

const Value* find(const Value& value, std::string_view key)
{
    if (!value.is_object()) {
        return nullptr;
    }
    const auto it = value.find(key);
    return it == value.end() ? nullptr : &*it;
}

std::string as_string_or(const Value& value, std::string_view fallback)
{
    return value.is_string() ? value.get<std::string>() : std::string(fallback);
}

int64_t as_int_or(const Value& value, int64_t fallback)
{
    if (value.is_number_unsigned()) {
        const auto v = value.get<uint64_t>();
        return v <= static_cast<uint64_t>(INT64_MAX) ? static_cast<int64_t>(v) : fallback;
    }
    if (value.is_number_integer()) {
        return value.get<int64_t>();
    }
    if (value.is_number_float()) {
        const auto v = value.get<double>();
        if (std::isfinite(v) && v >= -0x1p63 && v < 0x1p63) {
            return static_cast<int64_t>(v);
        }
    }
    return fallback;
}

} // namespace maa::ios::json
