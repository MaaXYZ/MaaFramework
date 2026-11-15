#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "MaaFramework/MaaAPI.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define CHECK(value)                                     \
    do {                                                 \
        if (!(value)) {                                  \
            std::cerr << "failed: " #value << std::endl; \
            return 1;                                    \
        }                                                \
    } while (false)

namespace
{
std::atomic<int> ordinary_events = 0;

void MAA_CALL ordinary_sink(void*, const char*, const char*, void*)
{
    ++ordinary_events;
}
}

int main(int argc, char** argv)
{
    const auto library = argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::path(argv[0]).parent_path() / PLUGIN_TEST_LIBRARY;
#ifdef _WIN32
    auto module = LoadLibraryW(library.c_str());
    CHECK(module);
    auto PluginTestCount = reinterpret_cast<int (*)(int)>(GetProcAddress(module, "PluginTestCount"));
#else
    auto module = dlopen(library.c_str(), RTLD_NOW | RTLD_LOCAL);
    CHECK(module);
    auto PluginTestCount = reinterpret_cast<int (*)(int)>(dlsym(module, "PluginTestCount"));
#endif
    CHECK(PluginTestCount);
    CHECK(MaaGlobalLoadPlugin(library.string().c_str()));
    CHECK(MaaGlobalLoadPlugin(library.string().c_str()));
    CHECK(PluginTestCount(0) == 0);
    auto* names = MaaStringListBufferCreate();
    CHECK(!MaaGlobalGetPluginList(nullptr));
    CHECK(MaaGlobalGetPluginList(names));
    CHECK(MaaStringListBufferSize(names) >= 1);
    MaaStringListBufferDestroy(names);

    auto* resource = MaaResourceCreate();
    CHECK(resource);
    CHECK(PluginTestCount(0) == 1);
    CHECK(PluginTestCount(3) == 1);
    CHECK(!MaaGlobalLoadPlugin(library.string().c_str()));
    CHECK(!MaaGlobalShutdownPlugins());
    MaaResourceAddSink(resource, ordinary_sink, nullptr);
    MaaResourceClearSinks(resource);
    const auto directory =
        std::filesystem::temp_directory_path() / ("maafw-plugin-test-" + std::to_string(reinterpret_cast<uintptr_t>(resource)));
    std::filesystem::create_directories(directory / "pipeline");
    std::ofstream(directory / "pipeline" / "test.json")
        << R"({"Entry":{"recognition":"DirectHit","action":"Custom","custom_action":"PluginTestAction"}})";
    const auto resource_id = MaaResourcePostBundle(resource, directory.string().c_str());
    CHECK(resource_id != MaaInvalidId);
    CHECK(MaaResourceWait(resource, resource_id) == MaaStatus_Succeeded);
    CHECK(PluginTestCount(9) == 2);
    CHECK(ordinary_events == 0);

    auto* tasker = MaaTaskerCreate();
    CHECK(tasker && PluginTestCount(5) == 1);
    CHECK(MaaTaskerBindResource(tasker, resource));
    MaaTaskerAddSink(tasker, ordinary_sink, nullptr);
    MaaTaskerClearSinks(tasker);
    MaaTaskerAddContextSink(tasker, ordinary_sink, nullptr);
    MaaTaskerClearContextSinks(tasker);
    const auto task_id = MaaTaskerPostTask(tasker, "Entry", "{}");
    CHECK(task_id != MaaInvalidId);
    CHECK(MaaTaskerWait(tasker, task_id) == MaaStatus_Succeeded);
    CHECK(PluginTestCount(10) == 1 && PluginTestCount(11) == 2);
    CHECK(PluginTestCount(13) == 1);
    CHECK(ordinary_events == 0);
    auto* controller = MaaDbgControllerCreate(directory.string().c_str());
    CHECK(controller && PluginTestCount(7) == 1);
    MaaControllerAddSink(controller, ordinary_sink, nullptr);
    MaaControllerClearSinks(controller);
    const auto connection_id = MaaControllerPostConnection(controller);
    CHECK(connection_id != MaaInvalidId);
    // The fixture has no screenshots, so the debug connection fails normally.
    CHECK(MaaControllerWait(controller, connection_id) == MaaStatus_Failed);
    CHECK(PluginTestCount(14) == 2 && ordinary_events == 0);
    MaaControllerDestroy(controller);
    CHECK(PluginTestCount(8) == 1);
    MaaTaskerDestroy(tasker);
    MaaResourceDestroy(resource);
    CHECK(PluginTestCount(6) == 1 && PluginTestCount(4) == 1);
    CHECK(MaaGlobalFlushPlugins() && PluginTestCount(1) == 1);
    CHECK(MaaGlobalShutdownPlugins() && PluginTestCount(2) == 1 && PluginTestCount(1) == 2);
    CHECK(MaaGlobalShutdownPlugins() && PluginTestCount(2) == 1);
    CHECK(PluginTestCount(12) == 2);
    CHECK(!MaaGlobalFlushPlugins());
    CHECK(!MaaGlobalLoadPlugin(library.string().c_str()));
    resource = MaaResourceCreate();
    CHECK(resource && PluginTestCount(3) == 1);
    MaaResourceDestroy(resource);
    std::filesystem::remove_all(directory);
#ifdef _WIN32
    FreeLibrary(module);
#else
    dlclose(module);
#endif
    std::cout << "Plugin lifecycle and sink isolation passed" << std::endl;
}
