#pragma once

#include <cstdint>
#include <meojson/json.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace maa::ios::json
{

using Value = ::json::value;

// Protocol inputs are limited to 4 MiB and 64 nested containers.
std::optional<Value> parse(std::string_view text, std::string* err = nullptr);
std::string write(const Value& value, bool replace_invalid_utf8 = false);
const Value* find(const Value& value, std::string_view key);
std::string as_string_or(const Value& value, std::string_view fallback = { });
std::optional<double> as_double(const Value& value);
int64_t as_int_or(const Value& value, int64_t fallback = 0);

} // namespace maa::ios::json
