#pragma once

#include <map>
#include <string>

#include "Conf/Conf.h"

#include <meojson/json.hpp>

MAA_TOOLKIT_NS_BEGIN

class Config // TODO:  public MaaToolKitConfigAPI
{
public:
    json::value to_json() const;
    bool from_json(const json::value& json);
};

MAA_TOOLKIT_NS_END
