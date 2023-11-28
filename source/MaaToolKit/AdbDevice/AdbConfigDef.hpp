#pragma once

#include "Conf/Conf.h"

#include <string_view>

#include <meojson/json.hpp>

MAA_TOOLKIT_DEVICE_NS_BEGIN

using namespace json::literals;
inline static const json::value kAdbConfig = "{}"_json;

MAA_TOOLKIT_DEVICE_NS_END
