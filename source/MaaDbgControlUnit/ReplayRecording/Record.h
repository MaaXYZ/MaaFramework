#pragma once

#include "Conf/Conf.h"

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include "Utils/NoWarningCVMat.hpp"

#include <meojson/json.hpp>

MAA_DBG_CTRL_UNIT_NS_BEGIN

struct DeviceInfo
{
    std::string uuid;
    cv::Size resolution {};
};

struct Record
{
    struct ClickParam
    {
        int x = 0;
        int y = 0;
    };
    struct SwipeParam
    {
        int x1 = 0;
        int y1 = 0;
        int x2 = 0;
        int y2 = 0;
        int duration = 0;
    };
    struct TouchParam
    {
        int contact = 0;
        int x = 0;
        int y = 0;
        int pressure = 0;
    };
    struct PressKeyParam
    {
        int keycode = 0;
    };
    struct AppParam
    {
        std::string package;
    };
    struct ScreencapParam
    {
        std::filesystem::path path;
        cv::Mat image;
    };

    using Param =
        std::variant<std::monostate, ClickParam, SwipeParam, TouchParam, PressKeyParam, AppParam, ScreencapParam>;

    struct Action
    {
        enum class Type
        {
            invalid,
            connect,
            click,
            swipe,
            touch_down,
            touch_move,
            touch_up,
            press_key,
            screencap,
            start_app,
            stop_app,
        } type = Type::invalid;

        Param param;
    };

    size_t timestamp = 0;
    Action action;
    bool success = false;
    int cost = 0;
};

struct Recording
{
    DeviceInfo device_info;
    std::vector<Record> records;
};

MAA_DBG_CTRL_UNIT_NS_END
