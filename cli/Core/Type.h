#pragma once

#include <meojson/json.hpp>

namespace maa::cli
{

// represent a project, like predefined options and entries.
struct Project
{
    struct Resource
    {
        std::string name;
        std::vector<std::string> path;
    };

    struct CaseOption
    {
        struct Case
        {
            std::string name;
            json::object param;
        };

        std::vector<Case> cases;
        std::string default_name;
    };

    // struct InjectOptionEntry
    // {
    //     enum Type
    //     {
    //         Number,
    //         String
    //     };

    //     std::vector<std::vector<std::string>> path;
    // };

    struct Task
    {
        std::string name;
        std::string task_entry;
        json::object task_param;
        std::vector<std::string> related_option;
    };

    struct Executor
    {
        std::string exec_path;
        std::vector<std::string> exec_param;

        // MEO_JSONIZATION(exec_path, MEO_OPT exec_param);
    };

    std::vector<Resource> resource;
    std::vector<Task> task;
    std::unordered_map<std::string, CaseOption> option;
    std::unordered_map<std::string, Executor> recognizer;
    std::unordered_map<std::string, Executor> action;
};

// represent config for a project, like option values and entry chains.
struct ProjectConfig
{
    struct Controller
    {
        struct Adb
        {
            std::string adb;
            std::string serial;

            std::string touch;
            std::string key;
            std::string screencap;

            json::object config;
        };

        struct Win32
        {
            std::string method;
            std::string class_name;
            std::string window_name;

            std::string touch;
            std::string key;
            std::string screencap;
        };

        std::string name;
        std::string type;

        Adb adb;
        Win32 win32;
    };

    struct Resource
    {
        std::string name;
    };

    struct Option
    {
        std::string name;
        std::string value;
    };

    struct Task
    {
        std::string name;
        std::vector<Option> option;
    };

    Controller controller;
    Resource resource;
    std::vector<Task> task;
};

// represent a instance of a project. only runtime.
struct ProjectInstance
{
};

// represent cli config
struct Config
{
};

}
