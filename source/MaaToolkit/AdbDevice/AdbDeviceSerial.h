#pragma once

#include <charconv>
#include <format>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace MaaNS::ToolkitNS::adb_device_detail
{
inline std::string canonical_serial(const std::string& serial)
{
    std::string_view view(serial);

    // 模拟器 console N / adb N+1 的固定约定（AVD 表内两种写法并存即来源于此）
    constexpr std::string_view kEmulatorPrefix = "emulator-";
    if (view.starts_with(kEmulatorPrefix)) {
        std::string_view num = view.substr(kEmulatorPrefix.size());
        int port = 0;
        auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), port);
        if (ec == std::errc { } && ptr == num.data() + num.size() && port > 0 && port < 65535) {
            return std::format("127.0.0.1:{}", port + 1);
        }
        return serial;
    }

    auto colon = view.rfind(':');
    if (colon == std::string_view::npos) {
        return serial;
    }

    std::string host(view.substr(0, colon));
    for (auto& ch : host) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch + ('a' - 'A'));
        }
    }
    if (host == "localhost") {
        host = "127.0.0.1";
    }
    else if (host == "::1") {
        host = "[::1]";
    }
    return std::format("{}:{}", host, view.substr(colon + 1));
}

inline std::vector<std::string>
    unique_uncovered_serials(const std::vector<std::string>& serials, const std::unordered_set<std::string>& exclude_canonical)
{
    std::unordered_set<std::string> seen;
    std::vector<std::string> result;
    for (const auto& serial : serials) {
        // Keep different spellings until one connects: an unregistered emulator-N
        // may need adb connect via its TCP alias before it becomes available.
        if (!exclude_canonical.contains(canonical_serial(serial)) && seen.emplace(serial).second) {
            result.emplace_back(serial);
        }
    }
    return result;
}
} // namespace MaaNS::ToolkitNS::adb_device_detail
