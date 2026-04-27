#pragma once

#ifndef __ANDROID__

#include "Base/UnitBase.h"
#include "Input/MtouchHelper.h"
#include "MaaUtils/NoWarningCVMat.hpp"

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
    explicit AndrowsExtras(std::filesystem::path agent_path = { });

    virtual ~AndrowsExtras() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ScreencapBase
    virtual bool init() override;
    virtual std::optional<cv::Mat> screencap() override;

public: // from ControlUnitSink
    virtual void on_app_started(const std::string& intent) override;
    virtual void on_app_stopped(const std::string& intent) override;
    virtual void on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur) override;

protected: // from MtouchHelper
    virtual bool request_display_info() override;

private:
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

    // Execute an ADB shell command and return the trimmed output.
    std::optional<std::string> adb_shell(const std::string& cmd);

    std::string app_package_;

    // Cached display IDs (empty = not yet queried or query failed)
    std::string display_id_;
    std::string event_id_;

    // ADB command generators for screencap
    ProcessArgvGenerator screencap_encode_argv_;

    // ADB shell command generator (used to query display/event IDs)
    ProcessArgvGenerator adb_shell_argv_;
};

MAA_CTRL_UNIT_NS_END

#endif
