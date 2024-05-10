module;

#include <coroutine>
#include <string>
#include <vector>

#include <MaaFramework/MaaAPI.h>
#include <MaaToolkit/MaaToolkitAPI.h>

export module MaaPP:Win32Device;

import :EventLoop;
import :Promise;
import :ControllerType;
import :String;

#include "MaaPP/maa/Win32Device.hpp"
