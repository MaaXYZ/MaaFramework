#pragma once

#include "Common/MaaConf.h"

#include <filesystem>

MAA_CTRL_UNIT_NS_BEGIN

std::string temp_name();
std::filesystem::path temp_path(const std::string& name);

MAA_CTRL_UNIT_NS_END
