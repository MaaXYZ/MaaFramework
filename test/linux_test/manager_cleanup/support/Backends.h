#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include "Base/UnitBase.h"

namespace ManagerCleanupTest
{
struct State
{
    bool connect_wayland = true;
    bool init_screencap = true;
    bool init_input = true;
    int wayland_connections = 0;
    int screencap_inits = 0;
    int input_inits = 0;
    int live_resources = 0;
    std::vector<std::string> events;
};

inline State state;

// A real descriptor stands in for each backend's partially acquired resources.
// Only hardware backends are substituted; the manager and its public API are real.
class Resource
{
public:
    explicit Resource(std::string name)
        : name_(std::move(name))
        , fd_(open("/dev/null", O_RDONLY | O_CLOEXEC))
    {
        if (fd_ < 0) {
            throw std::runtime_error("failed to create backend resource");
        }
        ++state.live_resources;
        state.events.push_back("create " + name_);
    }

    ~Resource()
    {
        close(fd_);
        --state.live_resources;
        state.events.push_back("destroy " + name_);
    }

private:
    std::string name_;
    int fd_;
};
}

MAA_CTRL_UNIT_NS_BEGIN

class WaylandClient
{
public:
    bool connect(const std::filesystem::path&)
    {
        ++ManagerCleanupTest::state.wayland_connections;
        return ManagerCleanupTest::state.connect_wayland;
    }

private:
    ManagerCleanupTest::Resource resource_ { "wayland" };
};

class TestScreencap : public ScreencapBase
{
public:
    TestScreencap() = default;
    TestScreencap(int, uint32_t) { }
    explicit TestScreencap(std::shared_ptr<WaylandClient> client)
        : client_(std::move(client))
    {
    }

    bool init() override
    {
        ++ManagerCleanupTest::state.screencap_inits;
        return ManagerCleanupTest::state.init_screencap;
    }
    bool screencap(cv::Mat&) override { return true; }

private:
    std::shared_ptr<WaylandClient> client_;
    ManagerCleanupTest::Resource resource_ { "screencap" };
};

class TestInput : public RelativeMoveInput
{
public:
    TestInput() = default;
    explicit TestInput(const std::string&) { }
    TestInput(const std::string&, int, int) { }
    explicit TestInput(std::shared_ptr<WaylandClient> client)
        : client_(std::move(client))
    {
    }

    bool init() override
    {
        ++ManagerCleanupTest::state.input_inits;
        return ManagerCleanupTest::state.init_input;
    }
    MaaControllerFeature get_features() const override { return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick; }
    bool click(int, int) override { return true; }
    bool swipe(int, int, int, int, int) override { return true; }
    bool touch_down(int, int, int, int) override { return true; }
    bool touch_move(int, int, int, int) override { return true; }
    bool touch_up(int) override { return true; }
    bool click_key(int) override { return true; }
    bool input_text(const std::string&) override { return true; }
    bool key_down(int) override { return true; }
    bool key_up(int) override { return true; }
    bool scroll(int, int) override { return true; }
    bool relative_move(int, int) override { return true; }

private:
    std::shared_ptr<WaylandClient> client_;
    ManagerCleanupTest::Resource resource_ { "input" };
};

using NoneScreencap = TestScreencap;
using WlrScreencap = TestScreencap;
using PipeWireScreencap = TestScreencap;
using NoneInput = TestInput;
using WlrInput = TestInput;
using UInput = TestInput;
using EiInput = TestInput;

MAA_CTRL_UNIT_NS_END
