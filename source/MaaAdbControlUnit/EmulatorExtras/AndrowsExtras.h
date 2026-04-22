#pragma once

#ifndef __ANDROID__

#include "Base/UnitBase.h"
#include "Input/AdbShellInput.h"
#include "Input/MtouchHelper.h"
#include "Invoke/InvokeApp.h"
#include "MaaUtils/NoWarningCVMat.hpp"

#include <filesystem>
#include <optional>
#include <string>

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

// AndrowsExtras handles Androws (Tencent) emulator's virtual display.
// It dynamically queries the logical display ID via ADB shell at runtime,
// using the app package name to find the correct display.
// For touch input, it uses minitouch with a specific input device node
// that corresponds to the virtual display.
class AndrowsExtras
    : public ScreencapBase
    , public MtouchHelper
{
public:
    // agent_path: path to the minitouch agent directory.
    // Pass empty path (default) when only screencap functionality is needed.
    explicit AndrowsExtras(std::filesystem::path agent_path = {});

    virtual ~AndrowsExtras() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase
    virtual bool init() override;
    virtual std::optional<cv::Mat> screencap() override;

public: // from InputBase (via MtouchHelper)
    virtual MaaControllerFeature get_features() const override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

public: // from ControlUnitSink
    virtual void on_app_started(const std::string& intent) override;
    virtual void on_app_stopped(const std::string& intent) override;
    virtual void on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur) override;

protected: // from MtouchHelper
    virtual bool request_display_info() override;
    virtual std::pair<int, int> screen_to_touch(int x, int y) override { return _screen_to_touch(x, y); }
    virtual std::pair<int, int> screen_to_touch(double x, double y) override { return _screen_to_touch(x, y); }

private:
    template <typename T1, typename T2>
    inline std::pair<int, int> _screen_to_touch(T1 x, T2 y)
    {
        auto make_pair = [](double x, double y) {
            return std::make_pair(static_cast<int>(round(x)), static_cast<int>(round(y)));
        };
        switch (orientation_) {
        case 0:
            return make_pair(x * xscale_, y * yscale_);
        case 1:
            return make_pair(touch_height_ - y * yscale_, x * xscale_);
        case 2:
            return make_pair(touch_width_ - x * xscale_, touch_height_ - y * yscale_);
        case 3:
            return make_pair(y * yscale_, touch_width_ - x * xscale_);
        default:
            return make_pair(x * xscale_, y * yscale_);
        }
    }

    // Query logical display_id via ADB shell, cache result.
    // Returns true if display ID was successfully resolved.
    bool query_display_id();
    void clear_display_id_cache();

    // Query the input event device ID for the virtual display.
    bool query_event_id();

    // Push, chmod and invoke minitouch with the resolved event device.
    bool init_minitouch();
    // Reinvoke the already-pushed minitouch without re-pushing the binary.
    // Used when app/display/resolution changes but the binary has not changed.
    bool reinvoke_minitouch();
    void remove_binary();

    // Execute an ADB shell command and return the trimmed output.
    std::optional<std::string> adb_shell(const std::string& cmd);

    std::filesystem::path agent_path_;
    std::vector<std::string> arch_list_;

    std::string app_package_;

    // Cached display IDs (empty = not yet queried or query failed)
    std::string display_id_;
    std::string event_id_;

    // ADB command generators for screencap
    ProcessArgvGenerator screencap_encode_argv_;

    // ADB shell command generator (used to query display/event IDs)
    ProcessArgvGenerator adb_shell_argv_;

    std::shared_ptr<InvokeApp> invoke_app_ = std::make_shared<InvokeApp>();
    std::shared_ptr<AdbShellInput> adb_shell_input_ = std::make_shared<AdbShellInput>();
};

MAA_CTRL_UNIT_NS_END

#endif
