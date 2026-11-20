#pragma once

#include <functional>
#include <optional>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "MaaUtils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

// A successful Desktop selection contains a null HWND.
std::optional<HWND> resolve_bind_target(const json::object& target, const std::function<bool()>& cancelled);

MAA_CTRL_UNIT_NS_END
