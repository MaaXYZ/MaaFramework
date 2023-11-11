#pragma once

#include <meojson/json.hpp>

#include "ControlUnit/AdbControlUnitAPI.h"
#include "Platform/PlatformIO.h"
#include "Screencap/ScreencapHelper.h"
#include "Utils/ArgvWrapper.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class UnitBase
{
public:
    using Argv = ArgvWrapper<std::vector<std::string>>;

public:
    virtual ~UnitBase() = default;

    virtual bool parse(const json::value& config) = 0;

    virtual void set_io(std::shared_ptr<PlatformIO> io_ptr);
    virtual void set_replacement(Argv::replacement argv_replace);
    virtual void merge_replacement(Argv::replacement argv_replace, bool _override = true);

protected:
    static bool parse_argv(const std::string& key, const json::value& config, /*out*/ Argv& argv);

    std::optional<std::string> command(const Argv::value& cmd, bool recv_by_socket = false, int64_t timeout = 20000);

protected:
    std::shared_ptr<PlatformIO> io_ptr_ = nullptr;
    std::vector<std::shared_ptr<UnitBase>> children_;
    Argv::replacement argv_replace_;
};

class ScreencapBase : public UnitBase
{
public:
    virtual ~ScreencapBase() override = default;

public:
    virtual bool init(int swidth, int sheight) = 0;
    virtual void deinit() = 0;
    virtual bool set_wh(int swidth, int sheight) { return screencap_helper_.set_wh(swidth, sheight); }

    virtual std::optional<cv::Mat> screencap() = 0;

protected:
    ScreencapHelper screencap_helper_;
};

class TouchInputBase : virtual public UnitBase
{
public:
    virtual ~TouchInputBase() override = default;

public:
    virtual bool init(int swidth, int sheight, int orientation) = 0;
    virtual void deinit() = 0;
    virtual bool set_wh(int swidth, int sheight, int orientation) = 0;

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

public:
    virtual bool press_key(int key) = 0;
};

MAA_CTRL_UNIT_NS_END
