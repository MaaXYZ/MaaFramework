#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "Backends.h"
#include "Manager/LinuxControlUnitMgr.h"

using namespace MAA_CTRL_UNIT_NS;
using ManagerCleanupTest::state;

namespace
{
void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

size_t count_fds()
{
    const auto entries = std::filesystem::directory_iterator("/proc/self/fd");
    return std::distance(begin(entries), end(entries));
}

LinuxControlUnitConfig pipewire_config()
{
    LinuxControlUnitConfig config;
    config.screencap_method = MaaLinuxScreencapMethod_PipeWire;
    config.pw_node_id = 123;
    config.input_method = MaaLinuxInputMethod_UInput;
    return config;
}

LinuxControlUnitConfig wayland_config()
{
    LinuxControlUnitConfig config;
    config.screencap_method = MaaLinuxScreencapMethod_Wlr;
    config.input_method = MaaLinuxInputMethod_Wlr;
    return config;
}

void require_disconnected(LinuxControlUnitMgr& manager)
{
    require(!manager.connected(), "failed manager is marked connected");
    require(state.live_resources == 0, "failed connect retains backend resources");
    require(manager.get_features() == MaaControllerFeature_None, "failed manager exposes input features");
    require(!manager.click(0, 0), "failed manager retains callable input");
    cv::Mat image;
    require(!manager.screencap(image), "failed manager retains callable screencap");
}

void input_init_failure()
{
    state.init_input = false;
    LinuxControlUnitMgr manager(pipewire_config());
    const auto baseline = count_fds();
    for (int attempt = 0; attempt < 8; ++attempt) {
        require(!manager.connect(), "input failure reported success");
        require_disconnected(manager);
        require(count_fds() == baseline, "input failure retains descriptors");
    }
    require(state.screencap_inits == 8 && state.input_inits == 8, "failure was not injected after capture initialization");
}

void screencap_init_failure()
{
    state.init_screencap = false;
    LinuxControlUnitMgr manager(pipewire_config());
    require(!manager.connect(), "screencap failure reported success");
    require_disconnected(manager);
    require(state.input_inits == 0, "input initialized after screencap failure");
}

void input_creation_failure()
{
    auto config = pipewire_config();
    config.input_method = -1;
    LinuxControlUnitMgr manager(config);
    require(!manager.connect(), "invalid input method reported success");
    require_disconnected(manager);
    require(state.screencap_inits == 0, "capture initialized after input creation failure");
}

void screencap_creation_failure()
{
    auto config = wayland_config();
    config.screencap_method = -1;
    LinuxControlUnitMgr manager(config);
    require(!manager.connect(), "invalid screencap method reported success");
    require_disconnected(manager);
    require(state.wayland_connections == 1, "failure did not follow Wayland creation");
}

void wayland_failure_retry()
{
    state.connect_wayland = false;
    LinuxControlUnitMgr manager(wayland_config());
    const auto baseline = count_fds();
    require(!manager.connect(), "Wayland failure reported success");
    require_disconnected(manager);
    require(!manager.connect(), "retry accepted failed Wayland client");
    require_disconnected(manager);
    require(state.wayland_connections == 2, "Wayland failure retry skipped connection");
    require(count_fds() == baseline, "Wayland failure retains descriptors");
    state.connect_wayland = true;
    require(manager.connect(), "Wayland recovery failed");
    require(manager.connected(), "Wayland recovery is marked disconnected");
    require(state.wayland_connections == 3, "Wayland recovery reused a failed client");
}

void failure_then_success()
{
    state.init_input = false;
    LinuxControlUnitMgr manager(pipewire_config());
    require(!manager.connect(), "injected failure reported success");
    require_disconnected(manager);
    state.init_input = true;
    require(manager.connect(), "retry did not recover");
    require(manager.connected() && state.live_resources == 2, "successful retry lost resources");
    require(manager.click(0, 0), "successful retry lost input");
}

void repeated_success_then_failure()
{
    const auto baseline = count_fds();
    {
        LinuxControlUnitMgr manager(wayland_config());
        for (int attempt = 0; attempt < 8; ++attempt) {
            require(manager.connect(), "repeated connection failed");
            require(state.live_resources == 3, "repeated connection accumulated resources");
            require(count_fds() == baseline + 3, "repeated connection accumulated descriptors");
        }
        state.init_input = false;
        require(!manager.connect(), "reconnect failure reported success");
        require_disconnected(manager);
        require(count_fds() == baseline, "failed reconnect retains descriptors");
    }
    require(state.live_resources == 0, "destructor retains resources after failure");
}

void destructor_after_success()
{
    const auto baseline = count_fds();
    {
        LinuxControlUnitMgr manager(wayland_config());
        require(manager.connect(), "connection failed");
        require(state.live_resources == 3, "successful manager released resources early");
    }
    require(state.live_resources == 0, "destructor retains successful connection resources");
    require(count_fds() == baseline, "destructor retains descriptors");
    require(state.events.back() == "destroy wayland", "Wayland destroyed before its consumers");
}
}

int main()
{
    const std::pair<const char*, void (*)()> tests[] = {
        { "input init failure", input_init_failure },
        { "screencap init failure", screencap_init_failure },
        { "input creation failure", input_creation_failure },
        { "screencap creation failure", screencap_creation_failure },
        { "Wayland failure and retry", wayland_failure_retry },
        { "failure then success", failure_then_success },
        { "repeated success then failure", repeated_success_then_failure },
        { "destructor after success", destructor_after_success },
    };
    int failures = 0;
    for (const auto& [name, run] : tests) {
        state = {};
        try {
            run();
            require(state.live_resources == 0, "test left resources after destruction");
            std::cout << "PASS " << name << '\n';
        }
        catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    }
    return failures == 0 ? 0 : 1;
}
