#pragma once

#include <meojson/json.hpp>

#include "ControlUnit/AdbControlUnitAPI.h"
#include "Platform/PlatformIO.h"
#include "Screencap/ScreencapHelper.h"
#include "Utils/ArgvWrapper.hpp"

MAA_ADB_CTRL_UNIT_NS_BEGIN

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

class DeviceListBase : public DeviceListAPI, public UnitBase
{
public:
    virtual ~DeviceListBase() override = default;
};

class ConnectionBase : public ConnectionAPI, public UnitBase
{
public:
    virtual ~ConnectionBase() override = default;
};

class ActivityBase : public ActivityAPI, public UnitBase
{
public:
    virtual ~ActivityBase() override = default;
};

class DeviceInfoBase : public DeviceInfoAPI, public UnitBase
{
public:
    virtual ~DeviceInfoBase() override = default;
};

class ScreencapBase : public ScreencapAPI, public UnitBase
{
public:
    virtual ~ScreencapBase() override = default;

public:
    virtual bool set_wh(int swidth, int sheight) override { return screencap_helper_.set_wh(swidth, sheight); }

protected:
    ScreencapHelper screencap_helper_;
};

class TouchInputBase : public TouchInputAPI, virtual public UnitBase
{
public:
    virtual ~TouchInputBase() override = default;
};

class KeyInputBase : public KeyInputAPI, virtual public UnitBase
{
public:
    virtual ~KeyInputBase() override = default;
};

MAA_ADB_CTRL_UNIT_NS_END
