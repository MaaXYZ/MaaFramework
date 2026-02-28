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

struct RecordScreencapData : RecordScreencap
{
    cv::Mat image;
};

struct Record
{
    using Param = std::variant<
        std::monostate,
        RecordConnect,
        RecordClick,
        RecordSwipe,
        std::vector<RecordSwipe>,
        RecordTouch,
        RecordKey,
        RecordInputText,
        RecordApp,
        RecordScreencapData,
        RecordScroll>;

    struct Action
    {
        RecordType type = RecordType::invalid;
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


MAA_CTRL_UNIT_NS_END
