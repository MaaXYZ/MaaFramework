#include "ProjectInterface/Configurator.h"

#include <ranges>
#include <unordered_map>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/StringMisc.hpp"
#include "ProjectInterface/Parser.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

namespace
{
MaaWin32ScreencapMethod parse_win32_screencap_method(const std::string& method)
{
    static const std::unordered_map<std::string, MaaWin32ScreencapMethod> mapping = {
        { "GDI", MaaWin32ScreencapMethod_GDI },
        { "FramePool", MaaWin32ScreencapMethod_FramePool },
        { "DXGI_DesktopDup", MaaWin32ScreencapMethod_DXGI_DesktopDup },
        { "DXGI_DesktopDup_Window", MaaWin32ScreencapMethod_DXGI_DesktopDup_Window },
        { "PrintWindow", MaaWin32ScreencapMethod_PrintWindow },
        { "ScreenDC", MaaWin32ScreencapMethod_ScreenDC },
    };

    if (auto it = mapping.find(method); it != mapping.end()) {
        return it->second;
    }
    return MaaWin32ScreencapMethod_None;
}

MaaWin32InputMethod parse_win32_input_method(const std::string& method)
{
    static const std::unordered_map<std::string, MaaWin32InputMethod> mapping = {
        { "Seize", MaaWin32InputMethod_Seize },
        { "SendMessage", MaaWin32InputMethod_SendMessage },
        { "PostMessage", MaaWin32InputMethod_PostMessage },
        { "LegacyEvent", MaaWin32InputMethod_LegacyEvent },
        { "PostThreadMessage", MaaWin32InputMethod_PostThreadMessage },
        { "SendMessageWithCursorPos", MaaWin32InputMethod_SendMessageWithCursorPos },
        { "PostMessageWithCursorPos", MaaWin32InputMethod_PostMessageWithCursorPos },
        { "Gamepad", MaaWin32InputMethod_Gamepad },
    };

    if (auto it = mapping.find(method); it != mapping.end()) {
        return it->second;
    }
    return MaaWin32InputMethod_None;
}
} // namespace

bool Configurator::load(const std::filesystem::path& resource_dir, const std::filesystem::path& user_dir)
{
    LogFunc << VAR(resource_dir) << VAR(user_dir);

    auto data_opt = Parser::parse_interface(resource_dir / kInterfaceFilename);
    if (!data_opt) {
        LogError << "Failed to parse interface.json";
        return false;
    }
    data_ = *std::move(data_opt);
    if (data_.resource.empty()) {
        LogError << "Resource is empty";
        return false;
    }

    if (auto cfg_opt = Parser::parse_config(user_dir / kConfigPath)) {
        config_ = *std::move(cfg_opt);
        first_time_use_ = false;
    }
    else {
        first_time_use_ = true;
    }

    resource_dir_ = resource_dir;

    // 加载翻译文件
    load_translations();

    return true;
}

bool Configurator::check_configuration()
{
    LogFunc;

    if (first_time_use_) {
        return true;
    }

    return Parser::check_configuration(data_, config_);
}

void Configurator::save(const std::filesystem::path& user_dir)
{
    LogInfo << VAR(user_dir);

    const auto config_path = user_dir / kConfigPath;
    if (config_path.has_parent_path()) {
        std::filesystem::create_directories(config_path.parent_path());
    }

    std::ofstream ofs(config_path);
    if (!ofs.is_open()) {
        LogError << "failed to open" << VAR(config_path);
        return;
    }

    ofs << config_.to_json();
}

std::optional<RuntimeParam> Configurator::generate_runtime() const
{
    RuntimeParam runtime;

    auto resource_iter = std::ranges::find_if(data_.resource, [&](const auto& resource) { return resource.name == config_.resource; });

    if (resource_iter == data_.resource.end()) {
        LogWarn << "Resource not found";
        return std::nullopt;
    }

    for (const auto& path_string : resource_iter->path) {
        // v2: relative path from interface.json directory
        runtime.resource_path.emplace_back(resource_dir_ / MaaNS::path(path_string));
    }
    if (runtime.resource_path.empty()) {
        LogWarn << "No resource to load";
        return std::nullopt;
    }

    for (const auto& config_task : config_.task) {
        auto task_opt = generate_runtime_task(config_task);
        if (!task_opt) {
            LogWarn << "failed to generate runtime, ignore" << VAR(config_task.name);
            continue;
        }
        runtime.task.emplace_back(*std::move(task_opt));
    }
    if (runtime.task.empty()) {
        LogWarn << "No task to run";
        return std::nullopt;
    }

    auto controller_iter =
        std::ranges::find_if(data_.controller, [&](const auto& controller) { return controller.name == config_.controller.name; });
    if (controller_iter == data_.controller.end()) {
        LogWarn << "Controller not found" << VAR(config_.controller.name);
        return std::nullopt;
    }
    auto& controller = *controller_iter;

    switch (controller.type) {
    case InterfaceData::Controller::Type::Adb: {
        RuntimeParam::AdbParam adb;

        adb.name = config_.adb.name;
        adb.adb_path = config_.adb.adb_path;
        adb.address = config_.adb.address;
        adb.agent_path = MaaNS::path_to_utf8_string(resource_dir_ / "MaaAgentBinary");

        runtime.controller_param = std::move(adb);
    } break;

    case InterfaceData::Controller::Type::Win32: {
        RuntimeParam::Win32Param win32;

        win32.hwnd = config_.win32.hwnd;

        // v2: parse from config, use default if not specified or invalid
        if (!controller.win32.screencap.empty()) {
            win32.screencap = parse_win32_screencap_method(controller.win32.screencap);
        }
        if (win32.screencap == MaaWin32ScreencapMethod_None) {
            win32.screencap = MaaWin32ScreencapMethod_DXGI_DesktopDup;
        }

        if (!controller.win32.mouse.empty()) {
            win32.mouse = parse_win32_input_method(controller.win32.mouse);
        }
        if (win32.mouse == MaaWin32InputMethod_None) {
            win32.mouse = MaaWin32InputMethod_Seize;
        }

        if (!controller.win32.keyboard.empty()) {
            win32.keyboard = parse_win32_input_method(controller.win32.keyboard);
        }
        if (win32.keyboard == MaaWin32InputMethod_None) {
            win32.keyboard = MaaWin32InputMethod_Seize;
        }

        runtime.controller_param = std::move(win32);
    } break;

    case InterfaceData::Controller::Type::PlayCover: {
        RuntimeParam::PlayCoverParam playcover;

        playcover.address = config_.playcover.address;
        playcover.uuid = config_.playcover.uuid.empty() ? "maa.playcover" : config_.playcover.uuid;

        if (playcover.address.empty()) {
            LogError << "PlayCover address is empty";
            return std::nullopt;
        }

        runtime.controller_param = std::move(playcover);
    } break;

    default: {
        LogError << "Unknown controller type" << VAR(controller.type);
        return std::nullopt;
    }
    }

    // 设置分辨率配置
    runtime.display_config.short_side = controller.display_short_side;
    runtime.display_config.long_side = controller.display_long_side;
    runtime.display_config.raw = controller.display_raw;

    if (!data_.agent.child_exec.empty()) {
        RuntimeParam::Agent agent;

        agent.child_exec = MaaNS::path(data_.agent.child_exec);
        agent.child_args = data_.agent.child_args;
        agent.identifier = data_.agent.identifier;
        agent.cwd = resource_dir_;

        runtime.agent = std::move(agent);
    }

    return runtime;
}

std::optional<RuntimeParam::Task> Configurator::generate_runtime_task(const Configuration::Task& config_task) const
{
    auto data_iter = std::ranges::find_if(data_.task, [&](const auto& data_task) { return data_task.name == config_task.name; });
    if (data_iter == data_.task.end()) {
        LogWarn << "task not found" << VAR(config_task.name);
        return std::nullopt;
    }
    const auto& data_task = *data_iter;

    RuntimeParam::Task runtime_task { .name = data_task.name,
                                      .entry = data_task.entry,
                                      .pipeline_override = json::array { data_task.pipeline_override } };

    for (const auto& config_option : config_task.option) {
        auto data_option_iter = data_.option.find(config_option.name);
        if (data_option_iter == data_.option.end()) {
            LogWarn << "option not found" << VAR(config_option.name);
            continue;
        }
        const auto& data_option = data_option_iter->second;

        switch (data_option.type) {
        case InterfaceData::Option::Type::Select:
        case InterfaceData::Option::Type::Switch: {
            auto data_case_iter =
                std::ranges::find_if(data_option.cases, [&](const auto& data_case) { return data_case.name == config_option.value; });
            if (data_case_iter == data_option.cases.end()) {
                LogWarn << "case not found" << VAR(config_option.value);
                continue;
            }
            runtime_task.pipeline_override.emplace(data_case_iter->pipeline_override);
        } break;

        case InterfaceData::Option::Type::Input: {
            // Replace placeholders in pipeline_override with input values
            std::string override_str = data_option.pipeline_override.dumps();
            for (const auto& input_def : data_option.inputs) {
                std::string placeholder = "{" + input_def.name + "}";
                std::string value;
                if (auto it = config_option.inputs.find(input_def.name); it != config_option.inputs.end()) {
                    value = it->second;
                }
                else {
                    value = input_def.default_;
                }

                // Replace based on pipeline_type
                switch (input_def.pipeline_type) {
                case InterfaceData::Option::Input::PipelineType::String:
                    override_str = MaaNS::string_replace_all(override_str, "\"" + placeholder + "\"", "\"" + value + "\"");
                    override_str = MaaNS::string_replace_all(override_str, placeholder, value);
                    break;
                case InterfaceData::Option::Input::PipelineType::Int:
                case InterfaceData::Option::Input::PipelineType::Bool:
                    // For int/bool, replace the quoted placeholder with unquoted value
                    override_str = MaaNS::string_replace_all(override_str, "\"" + placeholder + "\"", value);
                    override_str = MaaNS::string_replace_all(override_str, placeholder, value);
                    break;
                }
            }
            if (auto parsed = json::parse(override_str)) {
                runtime_task.pipeline_override.emplace(parsed->as_object());
            }
            else {
                LogWarn << "Failed to parse pipeline override JSON for input option" << VAR(config_option.name) << VAR(override_str);
            }
        } break;
        }
    }

    return runtime_task;
}

std::string Configurator::detect_system_language() const
{
    // 尝试获取系统语言
    std::string lang;

#ifdef _WIN32
    // Windows: 使用 GetUserDefaultLocaleName
    wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
    if (GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH) > 0) {
        lang = MaaNS::from_u16(locale_name);
    }
#else
    // Unix-like: 使用环境变量
    if (const char* lc_all = std::getenv("LC_ALL"); lc_all && *lc_all) {
        lang = lc_all;
    }
    else if (const char* lc_messages = std::getenv("LC_MESSAGES"); lc_messages && *lc_messages) {
        lang = lc_messages;
    }
    else if (const char* language = std::getenv("LANG"); language && *language) {
        lang = language;
    }
#endif

    // 转换为小写并替换 - 为 _
    std::transform(lang.begin(), lang.end(), lang.begin(), [](unsigned char c) { return std::tolower(c); });
    std::replace(lang.begin(), lang.end(), '-', '_');

    // 提取语言代码 (例如 zh_cn.utf-8 -> zh_cn)
    if (auto pos = lang.find('.'); pos != std::string::npos) {
        lang = lang.substr(0, pos);
    }

    return lang;
}

void Configurator::load_translations()
{
    if (data_.languages.empty()) {
        return;
    }

    std::string sys_lang = detect_system_language();
    LogInfo << "System language:" << sys_lang;

    // 查找匹配的语言文件
    std::string translation_file;

    // 1. 精确匹配 (例如 zh_cn)
    if (auto it = data_.languages.find(sys_lang); it != data_.languages.end()) {
        translation_file = it->second;
    }
    // 2. 语言前缀匹配 (例如 zh_cn 匹配 zh)
    else if (auto pos = sys_lang.find('_'); pos != std::string::npos) {
        std::string lang_prefix = sys_lang.substr(0, pos);
        if (auto prefix_it = data_.languages.find(lang_prefix); prefix_it != data_.languages.end()) {
            translation_file = prefix_it->second;
        }
    }
    // 3. 尝试匹配任何以系统语言开头的语言（仅当 sys_lang 非空时）
    // 注意：starts_with("") 会匹配任何字符串，所以必须检查 sys_lang 是否为空
    if (translation_file.empty() && sys_lang.size() >= 2) {
        std::string lang_prefix = sys_lang.substr(0, 2);
        for (const auto& [lang_code, file] : data_.languages) {
            if (lang_code.starts_with(lang_prefix)) {
                translation_file = file;
                break;
            }
        }
    }

    if (translation_file.empty()) {
        LogInfo << "No matching translation file found for language:" << sys_lang;
        return;
    }

    // 加载翻译文件
    auto translation_path = resource_dir_ / translation_file;
    LogInfo << "Loading translation file:" << translation_path;

    std::ifstream ifs(translation_path);
    if (!ifs.is_open()) {
        LogWarn << "Failed to open translation file:" << translation_path;
        return;
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    auto json_opt = json::parse(buffer.str());
    if (!json_opt || !json_opt->is_object()) {
        LogWarn << "Failed to parse translation file:" << translation_path;
        return;
    }

    for (const auto& [key, value] : json_opt->as_object()) {
        if (value.is_string()) {
            translations_[key] = value.as_string();
        }
    }

    LogInfo << "Loaded" << translations_.size() << "translations";
}

std::string Configurator::translate(const std::string& text) const
{
    if (text.empty()) {
        return text;
    }

    // 如果文本以 $ 开头，则从翻译表中查找
    if (text[0] == '$') {
        std::string key = text.substr(1); // 去掉 $ 前缀
        if (auto it = translations_.find(key); it != translations_.end()) {
            return it->second;
        }
        // 如果没找到翻译，返回去掉 $ 的原文
        return key;
    }

    return text;
}

MAA_PROJECT_INTERFACE_NS_END
