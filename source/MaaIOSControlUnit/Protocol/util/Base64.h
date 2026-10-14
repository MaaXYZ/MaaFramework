#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace maa::ios::util
{
std::string base64_encode(std::string_view data);
std::string base64_encode(std::span<const uint8_t> data);
// 接受 MIME 空白和省略的末尾 padding；拒绝非法字符、错位 padding 及不完整字节。
std::optional<std::vector<uint8_t>> base64_decode(std::string_view text, std::string& err);
} // namespace maa::ios::util
