#pragma once

#include <filesystem>
#include <string>

#include <MaaToolkit/MaaToolkitDef.h>

struct MaaToolkitAdbDevice
{
public:
    virtual ~MaaToolkitAdbDevice() = default;

    virtual const std::string& name() const = 0;
    virtual const std::string& adb_path() const = 0;
    virtual const std::string& address() const = 0;
    virtual MaaAdbScreencapMethod screencap_methods() const = 0;
    virtual MaaAdbInputMethod input_methods() const = 0;
    virtual const std::string& config() const = 0;
};

struct MaaToolkitDesktopWindow
{
public:
    virtual ~MaaToolkitDesktopWindow() = default;

    virtual void* handle() const = 0;
    virtual const std::string& class_name() const = 0;
    virtual const std::string& window_name() const = 0;
};
