#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/JsonExt.hpp"

MAA_PROJECT_INTERFACE_NS_BEGIN

struct Executor
{
    std::string exec_path;
    std::vector<std::string> exec_param;

    MEO_JSONIZATION(exec_path, MEO_OPT exec_param);
};

struct InterfaceData
{
    struct Resource
    {
        std::string name;
        std::vector<std::string> path;

        MEO_JSONIZATION(name, path);
    };

    struct Task
    {
        std::string name;
        std::string entry;
        json::object param;
        std::vector<std::string> option;

        MEO_JSONIZATION(name, entry, MEO_OPT param, MEO_OPT option);
    };

    struct Option
    {
        struct Case
        {
            std::string name;
            json::object param;

            MEO_JSONIZATION(name, param);
        };

        std::vector<Case> cases;
        std::string default_case; // case.name

        MEO_JSONIZATION(cases, MEO_OPT default_case);
    };

    std::vector<Resource> resource;
    std::vector<Task> task;
    std::unordered_map<std::string, Option> option;
    std::unordered_map<std::string, Executor> recognizer;
    std::unordered_map<std::string, Executor> action;

    MEO_JSONIZATION(resource, task, MEO_OPT option, MEO_OPT recognizer, MEO_OPT action);
};

struct AdbControllerParam
{
    std::string adb_path;
    std::string address;
    MaaAdbControllerType type = MaaAdbControllerType_Input_Preset_Maatouch | MaaAdbControllerType_Screencap_FastestWay;
    std::string config;
    std::string agent_path;

    MEO_JSONIZATION(adb_path, address, type, config);
};

// TODO
// struct Win32Param
//{
//     MaaWin32Hwnd hwnd;
//     MaaWin32ControllerType type = MaaWin32ControllerType_Touch_SendMessage |
//                                   MaaWin32ControllerType_Key_SendMessage |
//                                   MaaWin32ControllerType_Screencap_DXGI_DesktopDup;
// };

struct Configuration
{
    struct Option
    {
        std::string name;
        std::string value;

        MEO_JSONIZATION(name, value);
    };

    struct Task
    {
        std::string name;
        std::vector<Option> option;

        MEO_JSONIZATION(name, MEO_OPT option);
    };

    AdbControllerParam adb_param;
    std::string resource;
    std::vector<Task> task;

    MEO_JSONIZATION(adb_param, resource, task);
};

struct RuntimeParam
{
    struct Task
    {
        std::string name;
        std::string entry;
        json::object param;

        MEO_JSONIZATION(name, entry, param);
    };

    std::vector<std::string> resource_path;
    // TODO: support win32
    AdbControllerParam adb_param;

    std::vector<Task> task;
    std::unordered_map<std::string, Executor> recognizer;
    std::unordered_map<std::string, Executor> action;

    MEO_JSONIZATION(resource_path, adb_param, task, recognizer, action);
};

MAA_PROJECT_INTERFACE_NS_END
