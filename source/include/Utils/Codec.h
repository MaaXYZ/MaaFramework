#pragma once

#include <string>
#include <string_view>

#include "Conf/Conf.h"
#include "MaaFramework/MaaPort.h"

MAA_NS_BEGIN

MAA_UTILS_API std::wstring to_u16(std::string_view u8str);
MAA_UTILS_API std::string from_u16(std::wstring_view u16str);

MAA_UTILS_API bool regex_valid(const std::wstring& regex);

MAA_NS_END
