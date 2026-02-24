#pragma once

#include <string>
#include <variant>
#include <vector>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "ControlUnit/RecordTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"

MAA_CTRL_UNIT_NS_BEGIN

struct DeviceInfo
{
    std::string uuid;
    cv::Size resolution {};
};

struct Record
{
    using ConnectParam = RecordConnect;
    using ClickParam = RecordClick;
    using SwipeParam = RecordSwipe;
    using TouchParam = RecordTouch;
    using ClickKeyParam = RecordKey;
    using InputTextParam = RecordInputText;
    using AppParam = RecordApp;
    using ScrollParam = RecordScroll;

    struct ScreencapParam : RecordScreencap
    {
        cv::Mat image;
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
    case Record::Action::Type::multi_swipe:
        os << "multi_swipe";
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
