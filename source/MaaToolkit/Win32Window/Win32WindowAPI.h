#pragma once

#include <string_view>
#include <vector>

#include "Conf/Conf.h"
#include "MaaToolkit/MaaToolkitDef.h"

class MaaWin32WindowAPI
{
public:
    virtual ~MaaWin32WindowAPI() = default;

    virtual size_t find_window(std::string_view class_name, std::string_view window_name) = 0;
    virtual size_t search_window(std::string_view class_name, std::string_view window_name) = 0;
    virtual std::vector<MaaWin32Hwnd> found_window() const = 0;
    virtual MaaWin32Hwnd get_cursor_window() const = 0;
    virtual MaaWin32Hwnd get_desktop_window() const = 0;
    virtual MaaWin32Hwnd get_foreground_window() const = 0;
};
