#if defined(__linux__) && !defined(ANDROID)

#include "DesktopWindowLinuxFinder.h"
#include <wayland-client-protocol.h>

#include <cstdlib>
#include <filesystem>

#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

std::vector<DesktopWindow> DesktopWindowLinuxFinder::find_all() const
{
    std::vector<DesktopWindow> result;

    const char* xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    if (!xdg_runtime_dir) {
        return result;
    }

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(xdg_runtime_dir, ec)) {
        if (ec) {
            LogWarn << "Failed to iterate XDG_RUNTIME_DIR: " << ec.message();
            break;
        }

        const auto& filename = entry.path().filename();
        if (filename.string().starts_with("wayland-") && !filename.string().ends_with(".lock")) {
            if (!entry.is_socket(ec) || ec) {
                LogWarn << "Cannot read status of file: " << entry << "msg: " << ec.message();
                ec.clear();
                continue;
            }

            auto display = wl_display_connect(filename.c_str());
            if (display == nullptr) {
                LogWarn << "Failed to connect socket: " << filename;
                continue;
            }

            uint32_t id = 0;
            if (int parsed = std::sscanf(filename.c_str(), "wayland-%d", &id); parsed != 1) {
                LogWarn << "Failed to parse wayland socket name: " << filename;
                wl_display_disconnect(display);
                continue;
            }
            result.emplace_back(
                DesktopWindow { .hwnd = reinterpret_cast<void*>(static_cast<uintptr_t>(id)),
                                .class_name = MAA_NS::to_u16(entry.path().string()),
                                .window_name = MAA_NS::to_u16(filename.string()) });

            wl_display_disconnect(display);
        }
    }
#ifdef MAA_DEBUG
    LogInfo << "Compositor list:" << result;
#endif
    return result;
}

MAA_TOOLKIT_NS_END

#endif // __linux__ && !ANDROID
