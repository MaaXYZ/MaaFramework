#pragma once

#include <string>

#include "MaaControlUnit/MaaControlUnitDef.h"

MAA_CTRL_UNIT_NS_BEGIN

namespace MacOSAppLifecycle
{
bool start_app(const std::string& intent, const std::string& search_path = "/Applications");
// Exit detection requires a running main run loop; call from a background thread.
bool stop_app(const std::string& intent);
}

MAA_CTRL_UNIT_NS_END
