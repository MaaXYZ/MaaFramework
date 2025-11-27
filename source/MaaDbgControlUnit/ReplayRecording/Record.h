#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "MaaUtils/NoWarningCVMat.hpp"

MAA_CTRL_UNIT_NS_BEGIN

struct DeviceInfo
{
    std::string uuid;
    cv::Size resolution {};
};

struct Record
{
    struct ConnectParam
    {
        std::string uuid;
        cv::Size resolution;
        std::string version;
    };

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
        int starting = 0;
    };

    struct TouchParam
    {
        int contact = 0;
        int x = 0;
        int y = 0;
        int pressure = 0;
    };

    struct ClickKeyParam
    {
        int keycode = 0;
    };

    struct InputTextParam
    {
        std::string text;
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

    struct ScrollParam
    {
        int dx = 0;
        int dy = 0;
    };

    using Param = std::variant<
        std::monostate,
        ConnectParam,
        ClickParam,
        SwipeParam,
        std::vector<SwipeParam>,
        TouchParam,
        ClickKeyParam,
        InputTextParam,
        AppParam,
        ScreencapParam,
        ScrollParam>;

    struct Action
    {
        enum class Type
        {
            invalid,
            connect,
            click,
            swipe,
            multi_swipe,
            touch_down,
            touch_move,
            touch_up,
            click_key,
            input_text,
            screencap,
            start_app,
            stop_app,
            key_down,
            key_up,
            scroll,
        } type = Type::invalid;

        Param param;
    };

    size_t timestamp = 0;
    Action action;
    bool success = false;
    int cost = 0;
    json::value raw_data;
};

struct Recording
{
    std::string version;
    DeviceInfo device_info;
    std::vector<Record> records;
};

inline std::ostream& operator<<(std::ostream& os, Record::Action::Type type)
{
    switch (type) {
    case Record::Action::Type::invalid:
        os << "invalid";
        break;
    case Record::Action::Type::connect:
        os << "connect";
        break;
    case Record::Action::Type::click:
        os << "click";
        break;
    case Record::Action::Type::swipe:
        os << "swipe";
        break;
    case Record::Action::Type::touch_down:
        os << "touch_down";
        break;
    case Record::Action::Type::touch_move:
        os << "touch_move";
        break;
    case Record::Action::Type::touch_up:
        os << "touch_up";
        break;
    case Record::Action::Type::click_key:
        os << "click_key";
        break;
    case Record::Action::Type::input_text:
        os << "input_text";
        break;
    case Record::Action::Type::screencap:
        os << "screencap";
        break;
    case Record::Action::Type::start_app:
        os << "start_app";
        break;
    case Record::Action::Type::stop_app:
        os << "stop_app";
        break;
    case Record::Action::Type::key_down:
        os << "key_down";
        break;
    case Record::Action::Type::key_up:
        os << "key_up";
        break;
    case Record::Action::Type::scroll:
        os << "scroll";
        break;
    default:
        os << "Unknown Record::Action::Type" << static_cast<int>(type);
        break;
    }
    return os;
}

MAA_CTRL_UNIT_NS_END
