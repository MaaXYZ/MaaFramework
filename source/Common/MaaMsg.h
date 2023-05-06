#pragma once

#include <functional>
#include <ostream>
#include <unordered_map>

#include <meojson/json.hpp>

#include "MaaConf.h"

MAA_NS_BEGIN

enum class MaaMsg : int64_t
{
    InvalidMsg = 0,
    ResourceMsg = 10000,
    ControllerMsg = 20000,
    InstanceMsg = 30000,

    TaskMsg = 100000,
    PipelineMsg = 1000000,
};

inline std::ostream& operator<<(std::ostream& os, const MaaMsg& type)
{
    static const std::unordered_map<MaaMsg, std::string> _type_name = {
        /* Global Msg */
        { MaaMsg::InvalidMsg, "InvalidMsg" },
        { MaaMsg::ResourceMsg, "ResourceMsg" },
        { MaaMsg::ControllerMsg, "ControllerMsg" },
        { MaaMsg::InstanceMsg, "InstanceMsg" },
        { MaaMsg::TaskMsg, "TaskMsg" },
        { MaaMsg::PipelineMsg, "PipelineMsg" },
    };
    return os << _type_name.at(type);
}

// 内部使用的回调
class MaaInstance;
using MaaInstCallback = std::function<void(MaaMsg msg, const json::value& details, MaaInstance* inst)>;

MAA_NS_END
