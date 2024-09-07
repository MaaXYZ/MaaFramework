#pragma once

#include <filesystem>
#include <regex>
#include <string_view>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "PI/Types.h"

MAA_PI_NS_BEGIN

class PIData : public MaaPIData
{
    friend class PIConfig;
    friend class PIRuntime;
    friend class PIClient;

public:
    struct ControllerInfo
    {
        struct AdbInfo
        {
            MaaAdbScreencapMethod screencap = MaaAdbScreencapMethod_Default;
            MaaAdbInputMethod input = MaaAdbInputMethod_Default;
            json::object config;

            bool from(const DataJson::Controller::AdbConfig& cfg);
        };

        struct DesktopInfo
        {
            std::regex class_regex;
            std::regex window_regex;

            MaaWin32ScreencapMethod screencap = MaaWin32ScreencapMethod_DXGI_DesktopDup;
            MaaWin32InputMethod input = MaaWin32InputMethod_Seize;

            bool from(const DataJson::Controller::DesktopConfig& cfg);
        };

        std::string name;
        I18nString name_i18n;
        std::variant<AdbInfo, DesktopInfo> info;

        bool from(const DataJson::Controller& cfg);
    };

    struct ResourceInfo
    {
        std::string name;
        I18nString name_i18n;
        std::vector<std::filesystem::path> paths;

        bool from(const DataJson::Resource& cfg, std::string_view path);
    };

    using TaskInfo = DataJson::Task;

    struct OptionInfo
    {
        using CaseInfo = DataJson::Option::Case;

        std::string name;
        I18nString name_i18n;
        std::unordered_map<std::string, CaseInfo> cases;
        std::string default_case;

        bool from(const DataJson::Option& cfg);
    };

    virtual bool load(std::string_view json, std::string_view path) override;

private:
    std::string resource_root_;

    std::unordered_map<std::string, ControllerInfo> controller_;
    std::vector<std::string> controller_order_;
    std::unordered_map<std::string, ResourceInfo> resource_;
    std::vector<std::string> resource_order_;
    std::unordered_map<std::string, TaskInfo> task_;
    std::vector<std::string> task_order_;
    std::unordered_map<std::string, OptionInfo> option_;

    // first of array
    std::string default_controller_;
    std::string default_resource_;
    std::string default_task_;
};

MAA_PI_NS_END
