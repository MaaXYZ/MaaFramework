#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_NS_BEGIN

inline BOOL CALLBACK collect_visible_window(HWND hwnd, LPARAM context)
{
    if (IsWindowVisible(hwnd)) {
        reinterpret_cast<std::vector<HWND>*>(context)->push_back(hwnd);
    }
    return TRUE;
}

inline std::vector<HWND> visible_top_level_windows()
{
    std::vector<HWND> windows;
    EnumWindows(collect_visible_window, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

struct Win32WindowNames
{
    std::wstring class_name;
    std::wstring window_name;
};

inline std::optional<Win32WindowNames> win32_window_names(HWND hwnd)
{
    std::array<wchar_t, 256> class_name;
    const int length = GetClassNameW(hwnd, class_name.data(), static_cast<int>(class_name.size()));
    if (length <= 0) {
        return std::nullopt;
    }

    DWORD pid = 0;
    const DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if (!tid) {
        return std::nullopt;
    }
    // Read the stored caption directly when another thread in this process owns the
    // window. GetWindowTextW sends WM_GETTEXT, which can deadlock if that thread is
    // waiting for this controller action. Foreign windows already use the stored caption.
    const bool local_ui_thread = pid == GetCurrentProcessId() && tid != GetCurrentThreadId();
    const auto caption_length = local_ui_thread ? DefWindowProcW(hwnd, WM_GETTEXTLENGTH, 0, 0) : GetWindowTextLengthW(hwnd);
    std::wstring title(static_cast<size_t>(std::clamp<LRESULT>(caption_length, 0, 32767)) + 1, L'\0');
    const auto copied = local_ui_thread ? DefWindowProcW(hwnd, WM_GETTEXT, title.size(), reinterpret_cast<LPARAM>(title.data()))
                                        : GetWindowTextW(hwnd, title.data(), static_cast<int>(title.size()));
    title.resize(static_cast<size_t>(std::clamp<LRESULT>(copied, 0, static_cast<LRESULT>(title.size() - 1))));
    return Win32WindowNames { std::wstring(class_name.data(), length), std::move(title) };
}

MAA_NS_END
