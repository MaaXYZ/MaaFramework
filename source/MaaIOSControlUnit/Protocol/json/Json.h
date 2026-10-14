#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace maa::ios::json
{

using Value = nlohmann::json;

// 协议输入保留 4 MiB / 64 层限制；语法与值存储由 nlohmann/json 提供。
std::optional<Value> parse(std::string_view text, std::string* err = nullptr);
std::string write(const Value& value);
const Value* find(const Value& value, std::string_view key);
std::string as_string_or(const Value& value, std::string_view fallback = { });
int64_t as_int_or(const Value& value, int64_t fallback = 0);

} // namespace maa::ios::json
