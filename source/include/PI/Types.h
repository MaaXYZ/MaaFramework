#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "MaaPI/MaaPIDef.h"

MAA_PI_NS_BEGIN

using I18nString = std::unordered_map<std::string, std::string>;

struct DataJson
{
    struct Controller
    {
        struct AdbConfig
        {
            std::vector<std::string> screencap;
            std::vector<std::string> input;
            json::object config;

            MEO_JSONIZATION(MEO_OPT screencap, MEO_OPT input, MEO_OPT config);
        };

        struct DesktopConfig
        {
            std::string class_regex;
            std::string window_regex;

            std::string screencap;
            std::string input;

            MEO_JSONIZATION(MEO_OPT class_regex, MEO_OPT window_regex, MEO_OPT screencap, MEO_OPT input);
        };

        std::string name;
        I18nString name_i18n;
        std::string type;
        AdbConfig adb;
        DesktopConfig desktop;

        MEO_JSONIZATION(name, MEO_OPT name_i18n, type, MEO_OPT adb, MEO_OPT desktop);
    };

    struct Resource
    {
        std::string name;
        I18nString name_i18n;
        std::vector<std::string> paths;

        MEO_JSONIZATION(name, MEO_OPT name_i18n, paths);
    };

    struct Task
    {
        std::string name;
        I18nString name_i18n;
        std::string entry;
        json::object pp_override;
        std::vector<std::string> option;

        MEO_JSONIZATION(name, MEO_OPT name_i18n, entry, MEO_OPT pp_override, MEO_OPT option);
    };

    struct Option
    {
        struct Case
        {
            std::string name;
            I18nString name_i18n;
            json::object pp_override;

            MEO_JSONIZATION(name, MEO_OPT name_i18n, pp_override);
        };

        std::string name;
        I18nString name_i18n;
        std::vector<Case> cases;
        std::string default_case;

        MEO_JSONIZATION(name, MEO_OPT name_i18n, cases, MEO_OPT default_case);
    };

    std::vector<Controller> controller;
    std::vector<Resource> resource;
    std::vector<Task> task;
    std::vector<Option> option;

    std::string version;
    I18nString message;

    MEO_JSONIZATION(controller, resource, task, MEO_OPT option, MEO_OPT version, MEO_OPT message);
};

struct ConfigJson
{
    struct AdbConfig
    {
        std::string adb_path = "adb";
        std::string address = "127.0.0.1:5555";
        json::object config = {};

        MEO_JSONIZATION(adb_path, address, config);
    };

    struct DesktopConfig
    {
        int _placeholder = 0;

        MEO_JSONIZATION(MEO_OPT _placeholder);
    };

    struct Task
    {
        struct Option
        {
            std::string name;
            std::string value;

            MEO_JSONIZATION(name, value);
        };

        std::string name;
        std::vector<Option> option;

        MEO_JSONIZATION(name, MEO_OPT option);
    };

    std::string controller;
    AdbConfig adb;
    DesktopConfig desktop;
    std::string resource;
    std::vector<Task> task;

    MEO_JSONIZATION(controller, MEO_OPT adb, MEO_OPT desktop, resource, task);
};

MAA_PI_NS_END

struct MaaPIData
{
public:
    virtual ~MaaPIData() = default;

    virtual bool load(std::string_view json, std::string_view path) = 0;
};

struct MaaPIConfig
{
public:
    virtual ~MaaPIConfig() = default;

    virtual bool load(MaaPIData* data, std::string_view json) = 0;
    virtual bool gen_default(MaaPIData* data) = 0;
    virtual bool save(std::string& json) = 0;
};

struct MaaPIRuntime
{
public:
    virtual ~MaaPIRuntime() = default;

    virtual bool bind(MaaPIData* data, MaaPIConfig* config) = 0;
    virtual bool setup(MaaNotificationCallback cb, void* cb_arg) = 0;
    virtual MaaController* get_controller() = 0;
    virtual MaaResource* get_resource() = 0;
    virtual MaaTasker* get_tasker() = 0;
};

struct MaaPIClient
{
public:
    virtual ~MaaPIClient() = default;

    virtual void setup(std::string_view locale, MaaPIClientHandler handler, void* handler_arg) = 0;
    virtual bool perform(MaaPIRuntime* rt, MaaPIClientAction action) = 0;
};
