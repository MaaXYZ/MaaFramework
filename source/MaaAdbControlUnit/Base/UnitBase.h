#pragma once

#include <chrono>

#include <meojson/json.hpp>

#include "Base/ProcessArgvGenerator.h"
#include "ControlUnit/AdbControlUnitAPI.h"
#include "Screencap/ScreencapHelper.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class UnitBase
{
public:
    using Replacement = ProcessArgvGenerator::Replacement;
    using ProcessArgv = ProcessArgvGenerator::ProcessArgv;

public:
    virtual ~UnitBase() = default;

    virtual bool parse(const json::value& config) = 0;

    virtual void set_replacement(Replacement argv_replace);
    virtual void merge_replacement(Replacement argv_replace, bool _override = true);

protected:
    static bool parse_command(
        const std::string& key,
        const json::value& config,
        const json::array& default_argv,
        /*out*/ ProcessArgvGenerator& argv);

    std::optional<std::string>
        startup_and_read_pipe(const ProcessArgv& argv, std::chrono::milliseconds timeout = std::chrono::milliseconds(20000));

protected:
    std::vector<std::shared_ptr<UnitBase>> children_;
    Replacement argv_replace_;
};

class ControlUnitSink
{
public:
    virtual ~ControlUnitSink() = default;

public:
    virtual void on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur)
    {
        std::ignore = pre;
        std::ignore = cur;
    }

    virtual void on_app_started(const std::string& intent) { std::ignore = intent; }

    virtual void on_app_stopped(const std::string& intent) { std::ignore = intent; }
};

class ScreencapBase
    : virtual public UnitBase
    , public ControlUnitSink
{
public:
    virtual ~ScreencapBase() override = default;

    virtual bool init() = 0;

public:
    virtual std::optional<cv::Mat> screencap() = 0;

protected:
    ScreencapHelper screencap_helper_;
};

class InputBase
    : virtual public UnitBase
    , public ControlUnitSink
{
public:
    virtual ~InputBase() override = default;

    virtual bool init() = 0;

public:
    virtual MaaControllerFeature get_features() const = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;

    virtual bool click_key(int key) = 0;
    virtual bool input_text(const std::string& text) = 0;

    virtual bool key_down(int key) = 0;
    virtual bool key_up(int key) = 0;

    virtual bool scroll(int dx, int dy) = 0;
};

MAA_CTRL_UNIT_NS_END
