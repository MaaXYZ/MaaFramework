#pragma once

#include <string>
#include <string_view>

#include "Conf/Conf.h"
#include "MaaFramework/MaaPort.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_NS_BEGIN

MAA_UTILS_API std::wstring to_u16(std::string_view u8str);
MAA_UTILS_API std::string from_u16(std::wstring_view u16str);

MAA_UTILS_API bool regex_valid(const std::wstring& regex);

MAA_UTILS_API std::string encode_image(const cv::Mat& image);
MAA_UTILS_API cv::Mat decode_image(const std::string& data);

MAA_NS_END
