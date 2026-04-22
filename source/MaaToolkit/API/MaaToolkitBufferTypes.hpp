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
    // 通用字段
    virtual uint64_t handle() const = 0;
    virtual const std::string& window_name() const = 0;
    // Win32 字段
    virtual const std::string& win32_class_name() const = 0;
    // MacOS 字段
    virtual int32_t macos_pid() const = 0;                         // macOS 进程ID
    virtual const std::string& macos_bundle_id() const = 0;        // macOS Bundle ID
    virtual const std::string& macos_application_name() const = 0; // macOS 应用程序名称
    // Linux 字段
    virtual const std::string& linux_socket_path() const = 0;
};
