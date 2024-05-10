module;

#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <ranges>
#include <string_view>
#include <utility>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <MaaToolkit/MaaToolkitAPI.h>
#include <meojson/json.hpp>

export module MaaPP:Instance;

import :Promise;
import :EventLoop;
import :Controller;
import :Resource;
import :String;
import :Image;
import :Message;
import :Type;
import :ActionHelper;
import :SyncContext;

#include "MaaPP/maa/Instance.hpp"
