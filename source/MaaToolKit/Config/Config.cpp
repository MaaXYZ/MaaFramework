#include "Config.h"

MAA_TOOLKIT_NS_BEGIN

json::value Config::to_json() const
{
    return json::value();
}

bool Config::from_json(const json::value& json)
{
    std::ignore = json;
    return false;
}

MAA_TOOLKIT_NS_END