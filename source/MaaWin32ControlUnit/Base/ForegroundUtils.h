#pragma once

#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

bool ensure_foreground_and_topmost(HWND hwnd);
bool ensure_foreground_with_cooldown(HWND hwnd);

MAA_CTRL_UNIT_NS_END
