#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace maa::ios::util
{
constexpr int hex_digit(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

std::string hex_encode(std::span<const uint8_t> bytes);
std::optional<std::vector<uint8_t>> hex_decode(std::string_view text, std::string& error);
bool is_hex(std::string_view text);
bool is_lower_hex(std::string_view text);
} // namespace maa::ios::util
