#pragma once

#include <string>

#include "Conf/Conf.h"
#include "MaaFramework/MaaPort.h"

MAA_NS_BEGIN

MAA_UTILS_API std::wstring to_u16(const std::string& u8str);
MAA_UTILS_API std::string from_u16(const std::wstring& u16str);

MAA_NS_END
