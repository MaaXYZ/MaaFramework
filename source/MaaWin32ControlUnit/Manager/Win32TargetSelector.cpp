#include "Win32TargetSelector.h"

#include <chrono>
#include <thread>

#include "Common/Win32WindowFinder.hpp"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

namespace
{
using Clock = std::chrono::steady_clock;

std::optional<HWND> find_window(
    const boost::wregex& class_regex,
    const boost::wregex& window_regex,
    std::chrono::milliseconds timeout,
    const std::function<bool()>& cancelled)
{
    const auto deadline = Clock::now() + timeout;
    do {
        HWND match = nullptr;
        for (HWND hwnd : visible_top_level_windows()) {
            if ((cancelled && cancelled()) || (timeout.count() > 0 && Clock::now() >= deadline)) {
                return std::nullopt;
            }
            auto names = win32_window_names(hwnd);
            if (!names || !boost::regex_search(names->class_name, class_regex) || !boost::regex_search(names->window_name, window_regex)) {
                continue;
            }
            if (match) {
                LogError << "Window selector is ambiguous" << VAR_VOIDP(match) << VAR_VOIDP(hwnd);
                return std::nullopt;
            }
            match = hwnd;
        }
        if (match) {
            return match;
        }
        if (cancelled && cancelled()) {
            return std::nullopt;
        }
        std::this_thread::sleep_until(std::min(deadline, Clock::now() + std::chrono::milliseconds(50)));
    } while (Clock::now() < deadline);
    LogError << "Window selector timed out" << VAR(timeout);
    return std::nullopt;
}
}

std::optional<HWND> resolve_bind_target(const json::object& target, const std::function<bool()>& cancelled)
{
    const auto win32 = target.find<json::object>("win32");
    const auto timeout = target.find<int>("bind_timeout");
    if (!win32 || (target.contains("bind_timeout") && (!timeout || *timeout < 0))) {
        LogError << "Invalid Win32 Bind parameters" << VAR(target);
        return std::nullopt;
    }
    for (const auto& [key, _] : *win32) {
        if (key != "method" && key != "class_regex" && key != "window_regex") {
            LogError << "Unknown Win32 selector field" << VAR(key);
            return std::nullopt;
        }
    }
    const auto method = win32->find<std::string>("method");
    const auto class_name = win32->find<std::string>("class_regex");
    const auto window_name = win32->find<std::string>("window_regex");
    if (!method || (win32->contains("class_regex") && !class_name) || (win32->contains("window_regex") && !window_name)) {
        LogError << "Invalid Win32 window selector" << VAR(target);
        return std::nullopt;
    }
    const bool has_pattern = (class_name && !class_name->empty()) || (window_name && !window_name->empty());
    if (*method == "Desktop" && !has_pattern) {
        return HWND { };
    }
    if (*method != "Window" || !has_pattern) {
        LogError << "Expected Desktop or Window with at least one nonempty regex" << VAR(target);
        return std::nullopt;
    }

    const auto class_regex = regex_valid(to_u16(class_name.value_or("")));
    const auto window_regex = regex_valid(to_u16(window_name.value_or("")));
    if (!class_regex || !window_regex) {
        return std::nullopt;
    }
    try {
        return find_window(*class_regex, *window_regex, std::chrono::milliseconds(timeout.value_or(20000)), cancelled);
    }
    catch (const std::runtime_error& error) {
        // Boost can reject a match at run time when it exceeds its complexity limit.
        LogError << "Window selector failed" << error.what();
        return std::nullopt;
    }
}

MAA_CTRL_UNIT_NS_END
