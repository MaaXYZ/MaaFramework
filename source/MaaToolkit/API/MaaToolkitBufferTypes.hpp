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

struct MaaToolkitGamescopeInstance
{
public:
    virtual ~MaaToolkitGamescopeInstance() = default;

    virtual uint32_t display_no() const = 0;
    virtual uint32_t pipewire_node_id() const = 0;
    virtual const std::string& eis_socket_path() const = 0;
};
