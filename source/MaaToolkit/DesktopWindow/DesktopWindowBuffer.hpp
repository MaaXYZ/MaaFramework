#pragma once

#include <ostream>

#include "API/MaaToolkitBufferTypes.hpp"
#include "Common/Conf.h"
#include "MaaUtils/Buffer/ListBuffer.hpp"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

struct DesktopWindow
{
#ifdef _WIN32
    void* hwnd = nullptr;
    std::string class_name;
    std::string window_name;

    MEO_TOJSON(hwnd, class_name, window_name);
#endif // _WIN32

#if defined(__APPLE__)
    uint32_t window_id = 0;       // macOS 窗口ID
    std::string title;            // macOS 窗口标题
    int32_t pid = 0;              // macOS 进程ID
    std::string bundle_id;        // macOS Bundle ID
    std::string application_name; // macOS 应用程序名称

    MEO_TOJSON(window_id, title, pid, bundle_id, application_name);
#endif // __APPLE__

#if defined(__linux__) && !defined(ANDROID)
    uint32_t wayland_id = 0;
    std::string wayland_socket_name;
    std::string wayland_socket_path;

    MEO_TOJSON(wayland_id, wayland_socket_name, wayland_socket_path);
#endif // __linux__ && !ANDROID
};

class DesktopWindowBuffer : public MaaToolkitDesktopWindow
{
public:
    DesktopWindowBuffer(const DesktopWindow& window)
        : window_(window)
    {
    }

    virtual ~DesktopWindowBuffer() override = default;

    // 通用字段
    virtual uint64_t handle() const override
    {
#ifdef _WIN32
        return reinterpret_cast<uintptr_t>(window_.hwnd);
#elif defined(__APPLE__)
        return window_.window_id;
#elif defined(__linux__) && !defined(ANDROID)
        return window_.wayland_id;
#endif
        return 0;
    }

    virtual const std::string& window_name() const override
    {
#ifdef _WIN32
        return window_.window_name;
#elif defined(__APPLE__)
        return window_.title;
#elif defined(__linux__) && !defined(ANDROID)
        return window_.wayland_socket_name;
#endif
        static const std::string kEmpty;
        return kEmpty;
    }

    // Win32 专有字段
    virtual const std::string& win32_class_name() const override
    {
#ifdef _WIN32
        return window_.class_name;
#endif
        LogError << "Only available on Win32";
        static const std::string kEmpty;
        return kEmpty;
    }

    // MacOS 专有字段
    virtual int32_t macos_pid() const override
    {
#ifdef __APPLE__
        return window_.pid;
#endif
        LogError << "Only available on MacOS";
        return 0;
    }

    virtual const std::string& macos_bundle_id() const override
    {
#ifdef __APPLE__
        return window_.bundle_id;
#endif
        LogError << "Only available on MacOS";
        static const std::string kEmpty;
        return kEmpty;
    }

    virtual const std::string& macos_application_name() const override
    {
#ifdef __APPLE__
        return window_.application_name;
#endif
        LogError << "Only available on MacOS";
        static const std::string kEmpty;
        return kEmpty;
    }

    // Linux 专有字段
    virtual const std::string& linux_socket_path() const override
    {
#if defined(__linux__) && !defined(ANDROID)
        return window_.wayland_socket_path;
#endif
        LogError << "Only available on Linux";
        static const std::string kEmpty;
        return kEmpty;
    }

private:
    DesktopWindow window_;
};

MAA_TOOLKIT_NS_END

struct MaaToolkitDesktopWindowList : public MAA_NS::ListBuffer<MAA_TOOLKIT_NS::DesktopWindowBuffer>
{
    virtual ~MaaToolkitDesktopWindowList() override = default;
};
