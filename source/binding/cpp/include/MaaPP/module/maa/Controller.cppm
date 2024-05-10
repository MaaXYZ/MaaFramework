module;

#include <functional>
#include <iostream>
#include <memory>
#include <ranges>
#include <string_view>
#include <utility>

#include <MaaFramework/MaaAPI.h>
#include <MaaFramework/MaaMsg.h>
#include <meojson/json.hpp>

export module MaaPP:Controller;

import :Promise;
import :EventLoop;
import :AdbDevice;
import :CustomController;
import :String;
import :Image;
import :Message;
import :Win32Device;
import :ActionHelper;

#include "MaaPP/maa/Controller.hpp"
