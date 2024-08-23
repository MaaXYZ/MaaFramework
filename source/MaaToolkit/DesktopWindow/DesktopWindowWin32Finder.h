#pragma once

#include "Utils/Platform.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class Win32WindowFinder
    : public MaaWin32WindowAPI
{
public:
public:
    virtual ~Win32WindowFinder() = default;

    virtual size_t find_window(std::string_view class_name, std::string_view window_name) override;
    virtual size_t
        search_window(std::string_view class_name, std::string_view window_name) override;
    virtual size_t list_windows() override;

    virtual std::vector<Window> found_windows() const override { return windows_; }

    virtual void* get_cursor_window() const override;
    virtual void* get_desktop_window() const override;
    virtual void* get_foreground_window() const override;

    virtual std::optional<std::string> get_class_name(void* hwnd) const override;
    virtual std::optional<std::string> get_window_name(void* hwnd) const override;

private:
    std::vector<Window> _list_windows() const;

private:
    std::vector<Window> windows_;
};

MAA_TOOLKIT_NS_END

std::ostream& operator<<(std::ostream& os, const MaaWin32WindowAPI::Window& window);
