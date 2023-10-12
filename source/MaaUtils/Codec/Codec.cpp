#ifdef _WIN32
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#include "Utils/Codec.h"

#include <codecvt>
#include <locale>

MAA_NS_BEGIN

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

MAA_NS_END
