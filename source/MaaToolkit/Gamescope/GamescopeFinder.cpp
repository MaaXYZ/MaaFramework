#if defined(__linux__) && !defined(ANDROID)

#include "GamescopeFinder.h"

#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <wayland-client.h>

#include "Protocol/gamescope-pipewire.h"

#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

namespace
{
constexpr std::string_view kGamescopePrefix = "gamescope-";
constexpr std::string_view kGamescopeEisSuffix = "-ei";
// SPA_ID_INVALID: gamescope 在 stream 未起时经 stream_node 回传该值, 统一归零表示"无节点"
constexpr uint32_t kInvalidNodeId = 0xFFFFFFFFu;

// 解析 "gamescope-<n>" 中的 display 号; 格式不符时返回 nullopt
std::optional<uint32_t> parse_display_no(std::string_view filename)
{
    if (!filename.starts_with(kGamescopePrefix)) {
        return std::nullopt;
    }

    std::string_view no = filename.substr(kGamescopePrefix.size());
    // gamescope 的 slot 上限为 128, 这里放宽到 9 位既远超实际又避免 stoul 溢出
    if (no.empty() || no.size() > 9 || !std::ranges::all_of(no, [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
        return std::nullopt;
    }

    return static_cast<uint32_t>(std::stoul(std::string(no)));
}

std::filesystem::path runtime_dir()
{
    if (const char* xdg = std::getenv("XDG_RUNTIME_DIR"); xdg && *xdg) {
        return xdg;
    }
    return std::filesystem::path("/run/user") / std::to_string(getuid());
}

struct PipewireQueryContext
{
    wl_registry* registry = nullptr;
    gamescope_pipewire* pipewire = nullptr;
    uint32_t node_id = 0;
};

void handle_global(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t /*version*/)
{
    auto* ctx = static_cast<PipewireQueryContext*>(data);
    if (std::strcmp(interface, gamescope_pipewire_interface.name) == 0) {
        ctx->pipewire = static_cast<gamescope_pipewire*>(wl_registry_bind(registry, name, &gamescope_pipewire_interface, 1));
    }
}

void handle_global_remove(void* /*data*/, wl_registry* /*registry*/, uint32_t /*name*/)
{
}

void handle_stream_node(void* data, gamescope_pipewire* /*pipewire*/, uint32_t node_id)
{
    static_cast<PipewireQueryContext*>(data)->node_id = node_id;
}

const wl_registry_listener kRegistryListener = {
    .global = handle_global,
    .global_remove = handle_global_remove,
};

const gamescope_pipewire_listener kPipewireListener = {
    .stream_node = handle_stream_node,
};

// 连到 gamescope 的 Wayland display 并用 gamescope_pipewire 协议取 node id。
// connect 失败返回 nullopt (幽灵实例, 由调用方丢弃); 其余情况返回 0 (无节点) 或有效 id。
std::optional<uint32_t> query_pipewire_node_id(const std::string& display_path)
{
    wl_display* display = wl_display_connect(display_path.c_str());
    if (!display) {
        return std::nullopt;
    }

    PipewireQueryContext ctx;
    ctx.registry = wl_display_get_registry(display);
    wl_registry_add_listener(ctx.registry, &kRegistryListener, &ctx);
    wl_display_roundtrip(display);

    if (ctx.pipewire) {
        gamescope_pipewire_add_listener(ctx.pipewire, &kPipewireListener, &ctx);
        wl_display_roundtrip(display);
        if (ctx.node_id == kInvalidNodeId) {
            ctx.node_id = 0;
        }
    }

    if (ctx.pipewire) {
        gamescope_pipewire_destroy(ctx.pipewire);
    }
    wl_registry_destroy(ctx.registry);
    wl_display_disconnect(display);

    return ctx.node_id;
}
} // namespace

GamescopeFinder& GamescopeFinder::get_instance()
{
    static GamescopeFinder instance;
    return instance;
}

std::vector<GamescopeInstance> GamescopeFinder::find_all()
{
    std::vector<GamescopeInstance> instances;
    auto dir = runtime_dir();

    std::error_code ec = { };
    std::filesystem::directory_iterator it(dir, ec);
    if (ec) {
        LogWarn << "failed to open runtime dir" << VAR(dir) << VAR(ec.message());
        return instances;
    }

    for (const auto& entry : it) {
        if (!entry.is_socket(ec)) {
            continue;
        }

        auto display_no = parse_display_no(entry.path().filename().string());
        if (!display_no) {
            continue;
        }

        // stale socket 在 connect 阶段即被丢弃; connect 成功但无 pipewire 时 node_id 为 0, 实例仍保留
        auto node_id = query_pipewire_node_id(entry.path().string());
        if (!node_id) {
            continue;
        }

        GamescopeInstance instance;
        instance.display_no = *display_no;
        instance.pipewire_node_id = *node_id;

        // EIS socket 与 Wayland display 同名加 "-ei" 后缀, 二者 n 必然一致 (gamescope 由同一 wl_display_name 派生)
        auto eis_path = dir / (entry.path().filename().string() + std::string(kGamescopeEisSuffix));
        if (std::filesystem::is_socket(eis_path, ec)) {
            instance.eis_socket_path = eis_path.string();
        }

        LogInfo << "Found gamescope instance" << VAR(instance.display_no) << VAR(instance.pipewire_node_id)
                << VAR(instance.eis_socket_path);
        instances.push_back(std::move(instance));
    }

    std::ranges::sort(instances, [](const auto& lhs, const auto& rhs) { return lhs.display_no < rhs.display_no; });

    return instances;
}

MAA_TOOLKIT_NS_END

#else

#include "GamescopeFinder.h"

MAA_TOOLKIT_NS_BEGIN

GamescopeFinder& GamescopeFinder::get_instance()
{
    static GamescopeFinder instance;
    return instance;
}

std::vector<GamescopeInstance> GamescopeFinder::find_all()
{
    return { };
}

MAA_TOOLKIT_NS_END

#endif
