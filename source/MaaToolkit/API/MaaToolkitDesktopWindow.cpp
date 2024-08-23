#include "MaaToolkit/DesktopWindow/MaaToolkitDesktopWindow.h"

#include "DesktopWindow/DesktopWindowBuffer.hpp"
#include "DesktopWindow/DesktopWindowLinuxFinder.h"
#include "DesktopWindow/DesktopWindowMacOSFinder.h"
#include "DesktopWindow/DesktopWindowWin32Finder.h"
#include "Utils/Logger.h"

static MAA_TOOLKIT_NS::DesktopWindowFinder& finder()
{
    using namespace MAA_TOOLKIT_NS;

#if defined(_WIN32)
    return DesktopWindowWin32Finder::get_instance();
#elif defined(__APPLE__)
    return DesktopWindowMacOSFinder::get_instance();
#elif defined(__linux__)
    return DesktopWindowLinuxFinder::get_instance();
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

    return &list->at(index);
}

void* MaaToolkitDesktopWindowGetHandle(const MaaToolkitDesktopWindow* window)
{
    if (!window) {
        LogError << "window is null";
        return nullptr;
    }

    return window->handle();
}

const char* MaaToolkitDesktopWindowGetClassName(const MaaToolkitDesktopWindow* window)
{
    if (!window) {
        LogError << "window is null";
        return nullptr;
    }

    return window->class_name().c_str();
}

const char* MaaToolkitDesktopWindowGetWindowName(const MaaToolkitDesktopWindow* window)
{
    if (!window) {
        LogError << "window is null";
        return nullptr;
    }

    return window->window_name().c_str();
}
