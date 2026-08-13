#if defined(__linux__) && !defined(ANDROID)

#include "GamescopeEisSocketFinder.h"

#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string_view>

#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

namespace
{
constexpr std::string_view kGamescopeEisPrefix = "gamescope-";
constexpr std::string_view kGamescopeEisSuffix = "-ei";

// 解析 "gamescope-<n>-ei" 中的 display 号; 格式不符时返回 nullopt
std::optional<unsigned long> parse_display_no(std::string_view filename)
{
    if (!filename.starts_with(kGamescopeEisPrefix) || !filename.ends_with(kGamescopeEisSuffix)) {
        return std::nullopt;
    }

    std::string_view no = filename.substr(kGamescopeEisPrefix.size(), filename.size() - kGamescopeEisPrefix.size() - kGamescopeEisSuffix.size());
    if (no.empty() || !std::ranges::all_of(no, [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
        return std::nullopt;
    }

    return std::stoul(std::string(no));
}

std::filesystem::path runtime_dir()
{
    if (const char* xdg = std::getenv("XDG_RUNTIME_DIR"); xdg && *xdg) {
        return xdg;
    }
    return std::filesystem::path("/run/user") / std::to_string(getuid());
}
} // namespace

GamescopeEisSocketFinder& GamescopeEisSocketFinder::get_instance()
{
    static GamescopeEisSocketFinder instance;
    return instance;
}

std::vector<GamescopeEisSocket> GamescopeEisSocketFinder::find_all()
{
    std::vector<GamescopeEisSocket> sockets;
    auto dir = runtime_dir();

    std::error_code ec = {};
    std::filesystem::directory_iterator it(dir, ec);
    if (ec) {
        LogWarn << "failed to open runtime dir" << VAR(dir) << VAR(ec.message());
        return sockets;
    }

    for (const auto& entry : it) {
        // gamescope 崩溃可能遗留 stale socket, 此处不做存活校验
        if (!entry.is_socket(ec)) {
            continue;
        }
        auto display_no = parse_display_no(entry.path().filename().string());
        if (!display_no) {
            continue;
        }

        GamescopeEisSocket socket { .path = entry.path().string(), .display_no = *display_no };
        LogInfo << "Found gamescope EIS socket" << VAR(socket.path);
        sockets.push_back(std::move(socket));
    }

    std::ranges::sort(sockets, [](const auto& lhs, const auto& rhs) { return lhs.display_no < rhs.display_no; });

    return sockets;
}

MAA_TOOLKIT_NS_END

#else

#include "GamescopeEisSocketFinder.h"

MAA_TOOLKIT_NS_BEGIN

GamescopeEisSocketFinder& GamescopeEisSocketFinder::get_instance()
{
    static GamescopeEisSocketFinder instance;
    return instance;
}

std::vector<GamescopeEisSocket> GamescopeEisSocketFinder::find_all()
{
    return {};
}

MAA_TOOLKIT_NS_END

#endif
