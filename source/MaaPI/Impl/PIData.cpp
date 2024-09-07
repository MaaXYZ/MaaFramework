#include "Impl/PIData.h"

#include "MaaFramework/MaaAPI.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"

#include <regex>

MAA_PI_NS_BEGIN

static constexpr std::string normalize(std::string_view str)
{
    bool first = true;
    bool prev = false;
    std::string result;
    for (auto ch : str) {
        if (std::isupper(ch)) {
            if (!first) {
                if (!prev) {
                    result.push_back('-');
                }
            }
            else {
                first = false;
            }
            result.push_back(static_cast<char>(std::tolower(ch)));
            prev = true;
        }
        else {
            result.push_back(ch);
        }
    }
    return result;
}

bool PIData::ControllerInfo::AdbInfo::from(const DataJson::Controller::AdbConfig& cfg)
{
    static const std::unordered_map<std::string, MaaAdbScreencapMethod> screencap_map = {
        { "encode-to-file-and-pull", MaaAdbScreencapMethod_EncodeToFileAndPull },
        { "encode", MaaAdbScreencapMethod_Encode },
        { "raw-with-gzip", MaaAdbScreencapMethod_RawWithGzip },
        { "raw-by-netcat", MaaAdbScreencapMethod_RawByNetcat },
        { "minicap-direct", MaaAdbScreencapMethod_MinicapDirect },
        { "minicap-stream", MaaAdbScreencapMethod_MinicapStream },
        { "emulator-extras", MaaAdbScreencapMethod_EmulatorExtras },
        { "all", MaaAdbScreencapMethod_All },
        { "default", MaaAdbScreencapMethod_Default },
    };
    static const std::unordered_map<std::string, MaaAdbInputMethod> input_map = {
        { "adb-shell", MaaAdbInputMethod_AdbShell },
        { "minitouch-and-adb-key", MaaAdbInputMethod_MinitouchAndAdbKey },
        { "maatouch", MaaAdbInputMethod_Maatouch },
        { "emulator-extras", MaaAdbScreencapMethod_EmulatorExtras },
        { "all", MaaAdbInputMethod_All },
        { "default", MaaAdbInputMethod_Default },
    };

    if (cfg.screencap.empty()) {
        screencap = MaaAdbScreencapMethod_Default;
    }
    else {
        screencap = MaaAdbScreencapMethod_None;
        for (const auto& s : cfg.screencap) {
            auto it = screencap_map.find(normalize(s));
            if (it != screencap_map.end()) {
                screencap |= it->second;
            }
        }
        if (screencap == MaaAdbScreencapMethod_None) {
            return false;
        }
    }

    if (cfg.input.empty()) {
        input = MaaAdbInputMethod_Default;
    }
    else {
        input = MaaAdbInputMethod_None;
        for (const auto& s : cfg.input) {
            auto it = input_map.find(normalize(s));
            if (it != input_map.end()) {
                input |= it->second;
            }
        }
        if (input == MaaAdbInputMethod_None) {
            return false;
        }
    }

    config = cfg.config;

    return true;
}

bool PIData::ControllerInfo::DesktopInfo::from(const DataJson::Controller::DesktopConfig& cfg)
{
    static const std::unordered_map<std::string, MaaWin32ScreencapMethod> screencap_map = {
        { "gdi", MaaWin32ScreencapMethod_GDI },
        { "frame-pool", MaaWin32ScreencapMethod_FramePool },
        { "dxgi-desktop-dup", MaaWin32ScreencapMethod_DXGI_DesktopDup },
        { "default", MaaWin32ScreencapMethod_DXGI_DesktopDup },
    };
    static const std::unordered_map<std::string, MaaWin32InputMethod> input_map = {
        { "seize", MaaWin32InputMethod_Seize },
        { "send-message", MaaWin32InputMethod_SendMessage },
        { "default", MaaWin32InputMethod_Seize },
    };

    try {
        class_regex = std::regex(cfg.class_regex);
    }
    catch (const std::regex_error& err) {
        std::ignore = err;
        return false;
    }

    try {
        window_regex = std::regex(cfg.window_regex);
    }
    catch (const std::regex_error& err) {
        std::ignore = err;
        return false;
    }

    if (cfg.screencap.empty()) {
        screencap = MaaWin32ScreencapMethod_DXGI_DesktopDup;
    }
    else {
        auto it = screencap_map.find(normalize(cfg.screencap));
        if (it != screencap_map.end()) {
            screencap = it->second;
        }
        else {
            return false;
        }
    }

    if (cfg.input.empty()) {
        input = MaaWin32InputMethod_Seize;
    }
    else {
        auto it = input_map.find(normalize(cfg.screencap));
        if (it != input_map.end()) {
            input = it->second;
        }
        else {
            return false;
        }
    }

    return true;
}

bool PIData::ControllerInfo::from(const DataJson::Controller& cfg)
{
    name = cfg.name;
    name_i18n = cfg.name_i18n;

    auto ntype = normalize(cfg.type);

    if (ntype == "adb") {
        AdbInfo adb;
        if (!adb.from(cfg.adb)) {
            return false;
        }
        info = std::move(adb);
    }
    else if (ntype == "desktop") {
        DesktopInfo dsk;
        if (!dsk.from(cfg.desktop)) {
            return false;
        }
        info = std::move(dsk);
    }
    else {
        return false;
    }

    return true;
}

bool PIData::ResourceInfo::from(const DataJson::Resource& cfg, std::string_view path)
{
    name = cfg.name;
    name_i18n = cfg.name_i18n;

    for (const auto& p : cfg.paths) {
        paths.push_back(MAA_NS::path(MAA_NS::string_replace_all(p, "{PROJECT_DIR}", path)));
    }

    return true;
}

bool PIData::OptionInfo::from(const DataJson::Option& cfg)
{
    name = cfg.name;
    name_i18n = cfg.name_i18n;

    if (cfg.cases.empty()) {
        return false;
    }

    cases.clear();
    for (const auto& cs : cfg.cases) {
        cases[cs.name] = cs;
    }

    if (cfg.default_case.empty()) {
        default_case = cfg.cases[0].name;
    }
    else {
        if (!cases.contains(cfg.default_case)) {
            return false;
        }
        default_case = cfg.default_case;
    }

    return true;
}

bool PIData::load(std::string_view json, std::string_view path)
{
    resource_root_ = path;

    auto json_value = json::parse(json);
    if (!json_value) {
        return false;
    }

    DataJson data;
    if (!data.from_json(json_value.value())) {
        return false;
    }

    controller_.clear();
    controller_order_.clear();
    resource_.clear();
    resource_order_.clear();
    task_.clear();
    task_order_.clear();
    option_.clear();
    default_controller_.clear();
    default_resource_.clear();
    default_task_.clear();

    if (data.controller.empty()) {
        return false;
    }
    for (const auto& ctrl : data.controller) {
        ControllerInfo info;
        if (!info.from(ctrl)) {
            return false;
        }
        controller_[ctrl.name] = std::move(info);
        controller_order_.push_back(ctrl.name);
    }
    default_controller_ = controller_order_[0];

    if (data.resource.empty()) {
        return false;
    }
    for (const auto& res : data.resource) {
        ResourceInfo info;
        if (!info.from(res, resource_root_)) {
            return false;
        }
        resource_[res.name] = std::move(info);
        resource_order_.push_back(res.name);
    }
    default_resource_ = resource_order_[0];

    if (data.task.empty()) {
        return false;
    }
    for (const auto& tsk : data.task) {
        task_[tsk.name] = tsk;
        task_order_.push_back(tsk.name);
    }
    default_task_ = task_order_[0];

    for (const auto& opt : data.option) {
        OptionInfo info;
        if (!info.from(opt)) {
            return false;
        }
        option_[opt.name] = std::move(info);
    }

    return true;
}

MAA_PI_NS_END
