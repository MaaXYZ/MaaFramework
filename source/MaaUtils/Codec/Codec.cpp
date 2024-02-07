#ifdef _WIN32
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#include "Utils/Codec.h"

#include <locale>

MAA_NS_BEGIN

// NOTE: 这不是真的 u16，只是命名问题而已
// 换用了跨平台的转换方法
std::wstring to_u16(const std::string& u8str)
{
#ifdef _WIN32
    const std::codecvt<wchar_t, char, std::mbstate_t>& cvt = std::use_facet<
        std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale(
        ".65001")); // 美国(英语,UTF-8)，https://learn.microsoft.com/en-us/cpp/c-runtime-library/locale-names-languages-and-country-region-strings?view=msvc-170
#else
    const std::codecvt<wchar_t, char, std::mbstate_t>& cvt =
        std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale("C.UTF-8")); // 默认字符集(UTF-8)
#endif
    std::mbstate_t mb {};
    std::wstring t(u8str.length(), L'\0');
    const char* from_next;
    wchar_t* to_next;
    std::codecvt_base::result result =
        cvt.in(mb, &u8str[0], &u8str[u8str.length()], from_next, &t[0], &t[t.length()], to_next);
    if (result != std::codecvt_base::ok) {
        return std::wstring(u8str.length(), '?');
    }
    t.resize(to_next - &t[0]);
    return t;
}
// 这个是真的 utf-8
std::string from_u16(const std::wstring& u16str)
{
#ifdef _WIN32
    const std::codecvt<wchar_t, char, std::mbstate_t>& cvt = std::use_facet<
        std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale(
        ".65001")); // 美国(英语,UTF-8)，https://learn.microsoft.com/en-us/cpp/c-runtime-library/locale-names-languages-and-country-region-strings?view=msvc-170
#else
    const std::codecvt<wchar_t, char, std::mbstate_t>& cvt =
        std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale("C.UTF-8")); // 默认字符集(UTF-8)
#endif
    std::mbstate_t mb {};
    std::string t(u16str.length() * cvt.max_length(), L'\0');
    const wchar_t* from_next;
    char* to_next;
    std::codecvt_base::result result =
        cvt.out(mb, &u16str[0], &u16str[u16str.length()], from_next, &t[0], &t[t.length()], to_next);
    if (result != std::codecvt_base::ok) {
        return std::string(u16str.length(), '?');
    }
    t.resize(to_next - &t[0]);
    return t;
}

MAA_NS_END
