#ifndef __ANDROID__

#include "AndrowsExtras.h"

#include <charconv>
#include <cmath>
#include <format>
#include <sstream>

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"

MAA_CTRL_UNIT_NS_BEGIN

AndrowsExtras::AndrowsExtras(std::filesystem::path agent_path)
{
    agent_path_ = std::move(agent_path);
    // Only register invoke_app_ / adb_shell_input_ into children_ when we actually
    // need the input pathway (agent_path_ non-empty). For screencap-only instances
    // these sub-objects stay idle and should not participate in parse/init/replacement
    // propagation.
    if (!agent_path_.empty()) {
        invoke_app_ = std::make_shared<InvokeApp>();
        adb_shell_input_ = std::make_shared<AdbShellInput>();
        children_.emplace_back(invoke_app_);
        children_.emplace_back(adb_shell_input_);
    }
}

AndrowsExtras::~AndrowsExtras()
{
    remove_binary();
}

bool AndrowsExtras::parse(const json::value& config)
{
    bool enable = config.get("extras", "androws", "enable", false);
    if (!enable) {
        LogInfo << "extras.androws.enable is false, ignore";
        return false;
    }

    app_package_ = config.get("extras", "androws", "app_package", std::string { });
    LogInfo << VAR(app_package_);

    // ADB shell command generator: {ADB} -s {ADB_SERIAL} shell {ANDROWS_SHELL_CMD}
    static const json::array kAdbShellArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "{ANDROWS_SHELL_CMD}",
    };

    // Screencap using logical display ID directly
    static const json::array kScreencapEncodeArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "exec-out", "screencap -d {ANDROWS_DISPLAY_ID} -p",
    };

    bool ok = parse_command("AndrowsShell", config, kAdbShellArgv, adb_shell_argv_)
              && parse_command("ScreencapEncode", config, kScreencapEncodeArgv, screencap_encode_argv_);

    if (ok && !agent_path_.empty()) {
        ok = parse_minitouch_config(config);
    }

    return ok;
}

bool AndrowsExtras::init()
{
    if (!query_display_id()) {
        return false;
    }

    // If no agent_path is provided, this instance is screencap-only; skip minitouch setup.
    if (agent_path_.empty()) {
        return true;
    }

    if (!query_event_id()) {
        return false;
    }
    return init_minitouch();
}

std::optional<cv::Mat> AndrowsExtras::screencap()
{
    if (display_id_.empty() && !query_display_id()) {
        LogError << "Androws: display ID not available";
        return std::nullopt;
    }

    auto argv_opt = screencap_encode_argv_.gen(argv_replace_);
    if (!argv_opt) {
        LogError << "Androws: failed to generate screencap encode argv";
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        LogWarn << "Androws: screencap encode failed";
        return std::nullopt;
    }

    return screencap_helper_.process_data(*output_opt, ScreencapHelper::decode_png);
}

bool AndrowsExtras::request_display_info()
{
    if (display_id_.empty() && !query_display_id()) {
        LogError << "Androws: display_id_ is empty, cannot query display size";
        return false;
    }

    // Query virtual display resolution: wm size -d {display_id}
    // Output format: "Physical size: 1280x720" (may also have "Override size: ...")
    auto output = adb_shell(std::format("wm size -d {}", display_id_));
    if (!output || output->empty()) {
        LogWarn << "Androws: wm size -d failed, falling back to default display info";
        return MtouchHelper::request_display_info();
    }

    auto trim = [](std::string_view s) {
        while (!s.empty() && s.front() == ' ') {
            s.remove_prefix(1);
        }
        while (!s.empty() && s.back() == ' ') {
            s.remove_suffix(1);
        }
        return s;
    };
    // Parse width and height from output (take the last "WxH" pattern to prefer Override size)
    int w = 0;
    int h = 0;
    std::istringstream iss(*output);
    std::string line;
    while (std::getline(iss, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::string value_part = line.substr(pos + 1);
        auto xpos = value_part.find('x');
        if (xpos == std::string::npos) {
            continue;
        }
        auto ws = trim(std::string_view(value_part.data(), xpos));
        auto hs = trim(std::string_view(value_part.data() + xpos + 1, value_part.size() - xpos - 1));
        int tw = 0, th = 0;
        auto [p1, ec1] = std::from_chars(ws.data(), ws.data() + ws.size(), tw);
        auto [p2, ec2] = std::from_chars(hs.data(), hs.data() + hs.size(), th);
        if (ec1 != std::errc() || ec2 != std::errc()) {
            continue;
        }
        if (tw > 0 && th > 0) {
            w = tw;
            h = th;
        }
    }

    if (w <= 0 || h <= 0) {
        LogWarn << "Androws: failed to parse wm size output" << VAR(*output);
        return MtouchHelper::request_display_info();
    }

    display_width_ = w;
    display_height_ = h;
    orientation_ = 0; // Androws virtual display has no rotation

    LogInfo << "Androws: virtual display resolution" << VAR(display_id_) << VAR(display_width_) << VAR(display_height_);
    return true;
}

void AndrowsExtras::on_app_started(const std::string& intent)
{
    std::string package = intent;
    auto slash = intent.find('/');
    if (slash != std::string::npos) {
        package = intent.substr(0, slash);
    }

    if (!app_package_.empty() && package != app_package_) {
        LogInfo << "app_package changed" << VAR(app_package_) << VAR(package);
    }

    app_package_ = package;
    clear_display_id_cache();
    query_display_id();

    if (!agent_path_.empty()) {
        query_event_id();
        // App switch: display/event may have changed but the minitouch binary is
        // already pushed; only re-invoke it to refresh the connection.
        reinvoke_minitouch();
    }
}

void AndrowsExtras::on_app_stopped(const std::string& intent)
{
    std::ignore = intent;
    clear_display_id_cache();
}

void AndrowsExtras::on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur)
{
    std::ignore = pre;
    std::ignore = cur;

    // Resolution change may imply a display switch; invalidate cached display_id
    // and re-query it (needed for both screencap and input paths).
    clear_display_id_cache();
    query_display_id();

    // Input-side: also re-query event_id and reinvoke minitouch to refresh touch info.
    if (!agent_path_.empty()) {
        query_event_id();
        reinvoke_minitouch();
    }
}

bool AndrowsExtras::query_display_id()
{
    if (!display_id_.empty()) {
        return true;
    }

    std::optional<std::string> display_output;

    if (!app_package_.empty()) {
        display_output = adb_shell(
            "dumpsys activity activities | grep -A2 " + app_package_
            + " | grep mDisplayId | grep -o 'mDisplayId=[0-9]*' | head -1 | grep -o '[0-9]*'");
    }

    if (!display_output || display_output->empty()) {
        LogWarn << "Androws: failed to get display ID by package, falling back to second display";
        display_output = adb_shell("dumpsys display | grep -o 'mDisplayId=[0-9]*' | head -2 | tail -1 | grep -o '[0-9]*'");
    }

    if (!display_output || display_output->empty()) {
        LogError << "Androws: failed to get display ID";
        return false;
    }

    display_id_ = *display_output;
    LogInfo << "Androws" << VAR(display_id_);

    argv_replace_["{ANDROWS_DISPLAY_ID}"] = display_id_;
    return true;
}

void AndrowsExtras::clear_display_id_cache()
{
    display_id_.clear();
    event_id_.clear();
    argv_replace_.erase("{ANDROWS_DISPLAY_ID}");
}

bool AndrowsExtras::query_event_id()
{
    if (!event_id_.empty()) {
        return true;
    }

    if (display_id_.empty()) {
        LogError << "Androws: display_id_ is empty, cannot query event_id";
        return false;
    }

    // Find the EventHub device ID associated with the virtual display,
    // then resolve its /dev/input/eventN path.
    // Reference: MaaAssistantArknights config.json eventId command
    const std::string cmd = std::format(
        "EH_ID=$(dumpsys input | grep -E 'EventHub Devices:|Viewport.*displayId={0},' "
        "| grep -B1 'displayId={0},' | grep EventHub | head -1 | sed 's/.*\\[ *//;s/ *\\].*//'); "
        "dumpsys input | grep \"    ${{EH_ID}}: \" -A2 | grep Path | head -1 | sed 's/.*event//'",
        display_id_);

    auto output = adb_shell(cmd);
    if (!output || output->empty()) {
        LogError << "Androws: failed to query event_id for display_id:" << display_id_;
        return false;
    }

    event_id_ = *output;
    LogInfo << "Androws" << VAR(event_id_);
    return true;
}

bool AndrowsExtras::init_minitouch()
{
    if (!push_minitouch()) {
        return false;
    }

    const std::string extra = std::format("-i -d /dev/input/event{}", event_id_);
    pipe_ios_ = invoke_app_->invoke_bin(extra);
    if (!pipe_ios_) {
        return false;
    }

    return read_info();
}

bool AndrowsExtras::reinvoke_minitouch()
{
    if (event_id_.empty()) {
        LogWarn << "Androws: event_id_ is empty, skip reinvoke";
        return false;
    }

    const std::string extra = std::format("-i -d /dev/input/event{}", event_id_);
    pipe_ios_ = invoke_app_->invoke_bin(extra);
    if (!pipe_ios_) {
        LogError << "Androws: failed to reinvoke minitouch";
        return false;
    }

    return read_info();
}

std::optional<std::string> AndrowsExtras::adb_shell(const std::string& cmd)
{
    argv_replace_["{ANDROWS_SHELL_CMD}"] = cmd;

    auto argv_opt = adb_shell_argv_.gen(argv_replace_);
    argv_replace_.erase("{ANDROWS_SHELL_CMD}");

    if (!argv_opt) {
        LogError << "Androws: failed to generate adb shell argv for cmd:" << cmd;
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return std::nullopt;
    }

    std::string& output = *output_opt;
    while (!output.empty() && (output.back() == '\n' || output.back() == '\r' || output.back() == ' ')) {
        output.pop_back();
    }

    return output;
}

MAA_CTRL_UNIT_NS_END

#endif
