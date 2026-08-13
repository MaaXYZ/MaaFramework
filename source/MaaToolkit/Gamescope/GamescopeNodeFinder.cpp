#if defined(__linux__) && !defined(ANDROID)

#include "GamescopeNodeFinder.h"

#include <cstring>

#include <pipewire/pipewire.h>

#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

namespace
{
constexpr char kGamescopeNodeName[] = "gamescope";
constexpr char kGamescopeMediaClass[] = "Video/Source";
}

static void registry_global(
    void* data,
    uint32_t id,
    uint32_t /*permissions*/,
    const char* type,
    uint32_t /*version*/,
    const struct spa_dict* props)
{
    auto* nodes = static_cast<std::vector<GamescopeNode>*>(data);
    if (!type || std::strcmp(type, PW_TYPE_INTERFACE_Node) != 0 || !props) {
        return;
    }

    const char* node_name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    const char* media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (node_name && media_class && std::strcmp(node_name, kGamescopeNodeName) == 0
        && std::strcmp(media_class, kGamescopeMediaClass) == 0) {
        nodes->push_back(GamescopeNode { id, node_name });
        LogInfo << "Found gamescope PipeWire node" << VAR(id) << VAR(node_name);
    }
}

static void registry_global_remove(void* /*data*/, uint32_t /*id*/) {}

GamescopeNodeFinder& GamescopeNodeFinder::get_instance()
{
    static GamescopeNodeFinder instance;
    return instance;
}

std::vector<GamescopeNode> GamescopeNodeFinder::find_all()
{
    std::vector<GamescopeNode> nodes;

    // PipeWire global init is safe to call multiple times (uses an internal refcount).
    pw_init(nullptr, nullptr);

    pw_thread_loop* loop = pw_thread_loop_new("maa-gamescope-find", nullptr);
    if (!loop) {
        LogError << "Failed to create PipeWire thread loop";
        return nodes;
    }

    pw_context* context = pw_context_new(pw_thread_loop_get_loop(loop), nullptr, 0);
    if (!context) {
        LogError << "Failed to create PipeWire context";
        pw_thread_loop_destroy(loop);
        return nodes;
    }

    pw_core* core = pw_context_connect(context, nullptr, 0);
    if (!core) {
        LogError << "Failed to connect to session PipeWire daemon";
        pw_context_destroy(context);
        pw_thread_loop_destroy(loop);
        return nodes;
    }

    static const struct pw_registry_events registry_events = {
        .version = PW_VERSION_REGISTRY_EVENTS,
        .global = registry_global,
        .global_remove = registry_global_remove,
    };

    spa_hook registry_hook = { };
    pw_registry* registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(registry, &registry_hook, &registry_events, &nodes);

    // 让事件循环跑一小段, 收集 registry 的 global 枚举结果 (本地 daemon 毫秒级完成)
    pw_thread_loop_start(loop);
    pw_thread_loop_lock(loop);
    pw_thread_loop_timed_wait(loop, 1);
    pw_thread_loop_unlock(loop);
    pw_thread_loop_stop(loop);

    spa_hook_remove(&registry_hook);
    pw_proxy_destroy(reinterpret_cast<struct pw_proxy*>(registry));
    pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_thread_loop_destroy(loop);

    return nodes;
}

MAA_TOOLKIT_NS_END

#else

#include "GamescopeNodeFinder.h"

MAA_TOOLKIT_NS_BEGIN

GamescopeNodeFinder& GamescopeNodeFinder::get_instance()
{
    static GamescopeNodeFinder instance;
    return instance;
}

std::vector<GamescopeNode> GamescopeNodeFinder::find_all()
{
    return {};
}

MAA_TOOLKIT_NS_END

#endif
