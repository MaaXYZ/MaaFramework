module;

#include <memory>
#include <string>

#include <MaaFramework/MaaAPI.h>
#include <meojson/json.hpp>

export module MaaPP:SyncContext;

import :Promise;
import :EventLoop;
import :String;
import :Image;
import :Type;

#include "MaaPP/maa/SyncContext.hpp"
