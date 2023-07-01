#pragma once

#include "ControlUnitAPI.h"
#include "Platform/PlatformIO.h"
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

    std::optional<std::string> command(Argv::value cmd, bool recv_by_socket = false, int64_t timeout = 20000);

protected:
    std::shared_ptr<PlatformIO> io_ptr_ = nullptr;
    std::vector<std::shared_ptr<UnitBase>> children_;
    Argv::replacement argv_replace_;
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
};

class TouchInputBase : public TouchInputAPI, public UnitBase
{
public:
    virtual ~TouchInputBase() override = default;
};

class KeyInputBase : public KeyInputAPI, public UnitBase
{
public:
    virtual ~KeyInputBase() override = default;
};

MAA_CTRL_UNIT_NS_END
