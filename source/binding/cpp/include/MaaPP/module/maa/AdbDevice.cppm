module;

#include <coroutine>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

export module MaaPP:AdbDevice;

import :EventLoop;
import :Promise;
import :ControllerType;

#include "MaaPP/maa/AdbDevice.hpp"
