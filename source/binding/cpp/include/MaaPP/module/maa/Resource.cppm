module;

#include <functional>
#include <iostream>
#include <ranges>
#include <utility>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

export module MaaPP:Resource;

import :Promise;
import :EventLoop;
import :String;
import :Message;
import :ActionHelper;

#include "MaaPP/maa/Resource.hpp"
