#include "util/Hex.h"

#include <algorithm>

namespace maa::ios::util
{
std::string hex_encode(std::span<const uint8_t> bytes)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        result += digits[byte >> 4];
        result += digits[byte & 15];
    }
    return result;
}

std::optional<std::vector<uint8_t>> hex_decode(std::string_view text, std::string& error)
{
    if (text.size() % 2 != 0) {
        error = "Hex length must be even";
        return std::nullopt;
    }
    std::vector<uint8_t> bytes;
    bytes.reserve(text.size() / 2);
    for (std::size_t i = 0; i < text.size(); i += 2) {
        const int hi = hex_digit(text[i]), lo = hex_digit(text[i + 1]);
        if (hi < 0 || lo < 0) {
            error = "Invalid hex character";
            return std::nullopt;
        }
        bytes.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return bytes;
}

bool is_hex(std::string_view text)
{
    return std::ranges::all_of(text, [](char c) { return hex_digit(c) >= 0; });
}

bool is_lower_hex(std::string_view text)
{
    return std::ranges::all_of(text, [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}
} // namespace maa::ios::util
