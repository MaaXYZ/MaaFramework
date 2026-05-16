#include "MaaToolkit/DesktopWindow/MaaToolkitDesktopWindow.h"

#include "DesktopWindow/DesktopWindowAndroidFinder.h"
#include "DesktopWindow/DesktopWindowBuffer.hpp"
#include "DesktopWindow/DesktopWindowLinuxFinder.h"
#include "DesktopWindow/DesktopWindowMacOSFinder.h"
#include "DesktopWindow/DesktopWindowWin32Finder.h"
#include "MaaUtils/Logger.h"

static MAA_TOOLKIT_NS::DesktopWindowFinder& finder()
{
    using namespace MAA_TOOLKIT_NS;

#if defined(_WIN32)
    return DesktopWindowWin32Finder::get_instance();
#elif defined(__APPLE__)
    return DesktopWindowMacOSFinder::get_instance();
#elif defined(__linux__) && !defined(ANDROID)
    return DesktopWindowLinuxFinder::get_instance();
#elif defined(__linux__) && defined(ANDROID)
    return DesktopWindowAndroidFinder::get_instance();
#endif
}

MaaToolkitDesktopWindowList* MaaToolkitDesktopWindowListCreate()
{
    return new MaaToolkitDesktopWindowList;
}

void MaaToolkitDesktopWindowListDestroy(MaaToolkitDesktopWindowList* handle)
{
    if (handle) {
        delete handle;
    }
}

MaaBool MaaToolkitDesktopWindowFindAll(MaaToolkitDesktopWindowList* buffer)
{
    if (!buffer) {
        LogError << "buffer is null";
        return false;
    }

    auto windows = finder().find_all();
    for (const auto& w : windows) {
        buffer->append(MAA_TOOLKIT_NS::DesktopWindowBuffer(w));
    }

    return true;
}

MaaSize MaaToolkitDesktopWindowListSize(const MaaToolkitDesktopWindowList* list)
{
    if (!list) {
        LogError << "list is null";
        return 0;
    }

    return list->size();
}

const MaaToolkitDesktopWindow* MaaToolkitDesktopWindowListAt(const MaaToolkitDesktopWindowList* list, MaaSize index)
{
    if (!list) {
        LogError << "list is null";
        return nullptr;
    }
    if (index >= list->size()) {
        LogError << "out of range" << VAR(index) << VAR(list->size());
        return nullptr;
    }

    return &list->at(index);
}

// 通用字段
uint64_t MaaToolkitDesktopWindowGetHandle(const MaaToolkitDesktopWindow* window)
{
    if (!window) {
        LogError << "window is null";
        return 0;
    }

    return window->handle();
}

const char* MaaToolkitDesktopWindowGetWindowName(const MaaToolkitDesktopWindow* window)
{
    if (!window) {
        LogError << "window is null";
        return "";
    }

    return window->window_name().c_str();
}

// Win32 专有字段
const char* MaaToolkitDesktopWindowGetWin32ClassName(const MaaToolkitDesktopWindow* window)
{
    if (!window) {
        LogError << "window is null";
        return "";
    }

    return window->win32_class_name().c_str();
}

// MacOS 专有字段
int32_t MaaToolkitDesktopWindowGetMacOSPID(const MaaToolkitDesktopWindow* window)
{
    if (!window) {
        LogError << "window is null";
        return 0;
    }

    return window->macos_pid();
}

const char* MaaToolkitDesktopWindowGetMacOSBundleID(const MaaToolkitDesktopWindow* window)
{
    if (!window) {
        LogError << "window is null";
        return "";
    }

    return window->macos_bundle_id().c_str();
}

const char* MaaToolkitDesktopWindowGetMacOSApplicationName(const MaaToolkitDesktopWindow* window)
{
    if (!window) {
        LogError << "window is null";
        return "";
    }

    return window->macos_application_name().c_str();
}

// Linux 专有字段
const char* MaaToolkitDesktopWindowGetLinuxSocketPath(const MaaToolkitDesktopWindow* window)
{
    if (!window) {
        LogError << "window is null";
        return "";
    }

    return window->linux_socket_path().c_str();
}
