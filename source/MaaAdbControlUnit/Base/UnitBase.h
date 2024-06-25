#pragma once

#include <chrono>

#include <meojson/json.hpp>

#include "Base/ProcessArgvGenerator.h"
#include "ControlUnit/AdbControlUnitAPI.h"
#include "Screencap/ScreencapHelper.h"

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

    std::optional<std::string> startup_and_read_pipe(
        const ProcessArgv& argv,
        std::chrono::seconds timeout = std::chrono::seconds(20));

protected:
    std::vector<std::shared_ptr<UnitBase>> children_;
    Replacement argv_replace_;
};

class ScreencapBase : virtual public UnitBase
{
public:
    virtual ~ScreencapBase() override = default;

    virtual bool init() = 0;
    virtual void deinit() = 0;

public:
    virtual std::optional<cv::Mat> screencap() = 0;

protected:
    ScreencapHelper screencap_helper_;
};

class TouchInputBase : virtual public UnitBase
{
public:
    virtual ~TouchInputBase() override = default;

    virtual bool init() = 0;
    virtual void deinit() = 0;

public:
    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;
};

class KeyInputBase : virtual public UnitBase
{
public:
    virtual ~KeyInputBase() override = default;

    virtual bool init() = 0;
    virtual void deinit() = 0;

public:
    virtual bool press_key(int key) = 0;
    virtual bool input_text(const std::string& text) = 0;
};

MAA_CTRL_UNIT_NS_END
