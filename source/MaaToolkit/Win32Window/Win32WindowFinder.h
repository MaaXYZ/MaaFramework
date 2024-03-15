#pragma once

#include "Win32WindowAPI.h"

#include "Utils/Platform.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class Win32WindowFinder
    : public MaaWin32WindowAPI
    , public SingletonHolder<Win32WindowFinder>
{
public:
    struct Window
    {
        MaaWin32Hwnd hwnd;
        std::string class_name;
        std::string window_name;
    };

public:
    virtual ~Win32WindowFinder() = default;

    virtual size_t find_window(std::string_view class_name, std::string_view window_name) override;
    virtual size_t
        search_window(std::string_view class_name, std::string_view window_name) override;

    virtual std::vector<MaaWin32Hwnd> found_window() const override { return windows_; }

    virtual MaaWin32Hwnd get_cursor_window() const override;
    virtual MaaWin32Hwnd get_desktop_window() const override;
    virtual MaaWin32Hwnd get_foreground_window() const override;

private:
    std::vector<Window> list_windows() const;

private:
    std::vector<MaaWin32Hwnd> windows_;
};

std::ostream& operator<<(std::ostream& os, const Win32WindowFinder::Window& window);

MAA_TOOLKIT_NS_END
