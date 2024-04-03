#include "Utils/Codec.h"

#include <locale>

MAA_NS_BEGIN

std::wstring to_u16(const std::string& u8str)
{
#ifdef _WIN32
    // 美国(英语,UTF-8)，https://learn.microsoft.com/en-us/cpp/c-runtime-library/locale-names-languages-and-country-region-strings?view=msvc-170
    const std::codecvt<wchar_t, char, std::mbstate_t>& cvt =
        std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale(".65001"));
#else
    // 默认字符集(UTF-8)
    const std::codecvt<wchar_t, char, std::mbstate_t>& cvt =
        std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale("C.UTF-8"));
#endif
    std::mbstate_t mb {};
    std::wstring t(u8str.length(), L'\0');
    const char* from_next = nullptr;
    wchar_t* to_next = nullptr;
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
    // 美国(英语,UTF-8)，https://learn.microsoft.com/en-us/cpp/c-runtime-library/locale-names-languages-and-country-region-strings?view=msvc-170
    const std::codecvt<wchar_t, char, std::mbstate_t>& cvt =
        std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale(".65001"));
#else
    // 默认字符集(UTF-8)
    const std::codecvt<wchar_t, char, std::mbstate_t>& cvt =
        std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale("C.UTF-8"));
#endif
    std::mbstate_t mb {};
    std::string t(u16str.length() * cvt.max_length(), L'\0');
    const wchar_t* from_next = nullptr;
    char* to_next = nullptr;
    std::codecvt_base::result result = cvt.out(
        mb,
        &u16str[0],
        &u16str[u16str.length()],
        from_next,
        &t[0],
        &t[t.length()],
        to_next);
    if (result != std::codecvt_base::ok) {
        return std::string(u16str.length(), '?');
    }
    t.resize(to_next - &t[0]);
    return t;
}

MAA_NS_END
