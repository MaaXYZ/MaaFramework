#pragma once

#include <string>

#include "Conf/Conf.h"
#include "MaaFramework/MaaPort.h"

MAA_NS_BEGIN

std::wstring MAA_UTILS_API to_u16(const std::string& u8str);
std::string MAA_UTILS_API from_u16(const std::wstring& u16str);

MAA_NS_END
