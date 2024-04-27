#pragma once

#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

#include "MaaFramework/MaaDef.h"
#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/details/ControllerType.hpp"
#include "MaaPP/maa/details/String.hpp"
#include "MaaToolkit/Win32/MaaToolkitWin32Window.h"

namespace maa
{

struct Win32Type
    : public details::ControllerType<
          MaaWin32ControllerType,
          MaaWin32ControllerType_Touch_Mask,
          MaaWin32ControllerType_Key_Mask,
          MaaWin32ControllerType_Screencap_Mask,
          Win32Type>
{
    using ControllerType::ControllerType;
};

struct Win32Hwnd
{
    MaaWin32Hwnd hwnd_;

    Win32Hwnd(MaaWin32Hwnd hwnd = nullptr)
        : hwnd_(hwnd)
    {
    }

    std::string class_name() const
    {
        details::String buffer;
        MaaToolkitGetWindowClassName(hwnd_, buffer.handle());
        return buffer;
    }

    std::string window_name() const
    {
        details::String buffer;
        MaaToolkitGetWindowWindowName(hwnd_, buffer.handle());
        return buffer;
    }
};

struct Win32Device
{
    Win32Hwnd hwnd;
    Win32Type type;
};

namespace Win32DeviceFinder
{

namespace details
{

inline auto get_result(MaaSize size)
{
    std::vector<Win32Hwnd> result;
    result.reserve(size);
    for (size_t i = 0; i < size; i++) {
        result.push_back({ MaaToolkitGetWindow(i) });
    }
    return std::make_shared<std::vector<Win32Hwnd>>(std::move(result));
}

}

inline coro::Promise<std::shared_ptr<std::vector<Win32Hwnd>>>
    find(std::string class_name, std::string window_name)
{
    return coro::EventLoop::current()
        ->eval([class_name, window_name]() {
            return MaaToolkitFindWindow(class_name.c_str(), window_name.c_str());
        })
        .then([](auto size) { return details::get_result(size); });
}

inline coro::Promise<std::shared_ptr<std::vector<Win32Hwnd>>>
    search(std::string class_name, std::string window_name)
{
    return coro::EventLoop::current()
        ->eval([class_name, window_name]() {
            return MaaToolkitSearchWindow(class_name.c_str(), window_name.c_str());
        })
        .then([](auto size) { return details::get_result(size); });
}

inline Win32Hwnd cursor()
{
    return { MaaToolkitGetCursorWindow() };
}

inline Win32Hwnd desktop()
{
    return { MaaToolkitGetDesktopWindow() };
}

inline Win32Hwnd foreground()
{
    return { MaaToolkitGetForegroundWindow() };
}

}

}
