#ifdef _WIN32
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#include "Utils/Codec.h"

#include <codecvt>
#include <locale>

MAA_NS_BEGIN

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

std::wstring to_u16(const std::string& u8str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> code_converter;
    return code_converter.from_bytes(u8str);
}

std::string from_u16(const std::wstring& u16str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> code_converter;
    return code_converter.to_bytes(u16str);
}

#if defined(__clang__)
#pragma clang diagnostic pop // -Wdeprecated-declarations
#endif

MAA_NS_END