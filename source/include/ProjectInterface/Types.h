#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "MaaFramework/MaaDef.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

struct InterfaceData
{
    struct Controller
    {
        struct Win32Config
        {
            std::string class_regex;
            std::string window_regex;
            std::string screencap;
            std::string mouse;
            std::string keyboard;

            MEO_JSONIZATION(MEO_OPT class_regex, MEO_OPT window_regex, MEO_OPT screencap, MEO_OPT mouse, MEO_OPT keyboard);
        };

        struct PlayCoverConfig
        {
            std::string address;
            std::string uuid;

            MEO_JSONIZATION(MEO_OPT address, MEO_OPT uuid);
        };

        enum class Type
        {
            Invalid,
            Adb,
            Win32,
            PlayCover,
        };

        std::string name;
        std::string label;
        std::string description;
        Type type = Type::Adb;

        // 分辨率设置，三者互斥
        std::optional<int> display_short_side; // 默认720
        std::optional<int> display_long_side;
        bool display_raw = false;

        Win32Config win32;
        PlayCoverConfig playcover;

        MEO_JSONIZATION(
            name,
            MEO_OPT label,
            MEO_OPT description,
            type,
            MEO_OPT display_short_side,
            MEO_OPT display_long_side,
            MEO_OPT display_raw,
            MEO_OPT win32,
            MEO_OPT playcover);
    };

    struct Resource
    {
        std::string name;
        std::string label;
        std::string description;
        std::vector<std::string> path;
        std::vector<std::string> controller; // 支持的控制器列表

        MEO_JSONIZATION(name, MEO_OPT label, MEO_OPT description, path, MEO_OPT controller);
    };

    struct Task
    {
        std::string name;
        std::string label;
        std::string description;
        std::string entry;
        bool default_check = false;
        json::object pipeline_override;
        std::vector<std::string> option;
        std::vector<std::string> resource; // 支持的资源包列表

        MEO_JSONIZATION(
            name,
            MEO_OPT label,
            MEO_OPT description,
            entry,
            MEO_OPT default_check,
            MEO_OPT pipeline_override,
            MEO_OPT option,
            MEO_OPT resource);
    };

    struct Option
    {
        enum class Type
        {
            Select,
            Input,
            Switch,
        };

        struct Case
        {
            std::string name;
            std::string label;
            std::string description;
            json::object pipeline_override;
            std::vector<std::string> option; // 子选项

            MEO_JSONIZATION(name, MEO_OPT label, MEO_OPT description, MEO_OPT pipeline_override, MEO_OPT option);
        };

        struct Input
        {
            enum class PipelineType
            {
                String,
                Int,
                Bool,
            };

            std::string name;
            std::string label;
            std::string description;
            std::string default_;
            PipelineType pipeline_type = PipelineType::String;
            std::string verify;      // regex
            std::string pattern_msg; // 验证失败提示

            MEO_JSONIZATION(
                name,
                MEO_OPT label,
                MEO_OPT description,
                MEO_OPT MEO_KEY("default") default_,
                MEO_OPT pipeline_type,
                MEO_OPT verify,
                MEO_OPT pattern_msg);
        };

        Type type = Type::Select;
        std::string label;
        std::string description;
        std::vector<Case> cases;
        std::vector<Input> inputs;
        json::object pipeline_override; // for input type
        std::string default_case;       // case.name

        MEO_JSONIZATION(
            MEO_OPT type,
            MEO_OPT label,
            MEO_OPT description,
            MEO_OPT cases,
            MEO_OPT inputs,
            MEO_OPT pipeline_override,
            MEO_OPT default_case);
    };

    struct Agent
    {
        std::string child_exec;
        std::vector<std::string> child_args;
        std::string identifier;

        MEO_JSONIZATION(child_exec, MEO_OPT child_args, MEO_OPT identifier);
    };

    int interface_version = 2;
    std::unordered_map<std::string, std::string> languages; // 语言代码 -> 翻译文件路径
    std::string name;
    std::string label;
    std::string title;
    std::string version;
    std::string welcome;
    std::string description;
    std::string contact;
    std::string license;
    std::string github;

    std::vector<Controller> controller;
    std::vector<Resource> resource;
    std::vector<Task> task;
    std::unordered_map<std::string, Option> option;
    Agent agent;

    MEO_JSONIZATION(
        interface_version,
        MEO_OPT languages,
        MEO_OPT name,
        MEO_OPT label,
        MEO_OPT title,
        MEO_OPT version,
        MEO_OPT welcome,
        MEO_OPT description,
        MEO_OPT contact,
        MEO_OPT license,
        MEO_OPT github,
        controller,
        resource,
        task,
        MEO_OPT option,
        MEO_OPT agent);
};

struct Configuration
{
    struct Controller
    {
        std::string name;
        InterfaceData::Controller::Type type = InterfaceData::Controller::Type::Adb;

        MEO_JSONIZATION(name);
    };

    struct Win32Config
    {
        void* hwnd = nullptr;
        std::wstring class_name;
        std::wstring window_name;

        int _placeholder = 0;

        MEO_JSONIZATION(MEO_OPT _placeholder);
    };

    struct AdbConfig
    {
        std::string name;
        std::string adb_path;
        std::string address;

        MEO_JSONIZATION(name, adb_path, address);
    };

    struct PlayCoverConfig
    {
        std::string address;
        std::string uuid;

        MEO_JSONIZATION(MEO_OPT address, MEO_OPT uuid);
    };

    struct Option
    {
        std::string name;
        std::string value;                                   // for select/switch
        std::unordered_map<std::string, std::string> inputs; // for input type

        MEO_JSONIZATION(name, MEO_OPT value, MEO_OPT inputs);
    };

    struct Task
    {
        std::string name;
        std::vector<Option> option;

        MEO_JSONIZATION(name, MEO_OPT option);
    };

    Controller controller;
    AdbConfig adb;
    Win32Config win32;
    PlayCoverConfig playcover;
    std::string resource;
    std::vector<Task> task;

    MEO_JSONIZATION(controller, MEO_OPT adb, MEO_OPT win32, MEO_OPT playcover, resource, task);
};

struct RuntimeParam
{
    // 分辨率设置，三者互斥
    struct DisplayConfig
    {
        std::optional<int> short_side; // 默认720
        std::optional<int> long_side;
        bool raw = false;
    };

    struct AdbParam
    {
        std::string name;
        std::string adb_path;
        std::string address;
        MaaAdbScreencapMethod screencap = MaaAdbScreencapMethod_None;
        MaaAdbInputMethod input = MaaAdbInputMethod_None;
        std::string config;
        std::string agent_path;
    };

    struct Win32Param
    {
        void* hwnd = nullptr;
        MaaWin32ScreencapMethod screencap = MaaWin32ScreencapMethod_None;
        MaaWin32InputMethod mouse = MaaWin32InputMethod_None;
        MaaWin32InputMethod keyboard = MaaWin32InputMethod_None;
    };

    struct PlayCoverParam
    {
        std::string address;
        std::string uuid;
    };

    struct Task
    {
        std::string name;
        std::string entry;
        json::value pipeline_override;
    };

    struct Agent
    {
        std::filesystem::path child_exec;
        std::vector<std::string> child_args;
        std::string identifier;
        std::filesystem::path cwd;
    };

    std::variant<std::monostate, AdbParam, Win32Param, PlayCoverParam> controller_param;
    std::vector<std::filesystem::path> resource_path;

    std::vector<Task> task;

    std::optional<Agent> agent;
    DisplayConfig display_config;
};

MAA_PROJECT_INTERFACE_NS_END
