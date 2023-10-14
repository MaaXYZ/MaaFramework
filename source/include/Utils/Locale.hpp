#pragma once

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#ifdef _WIN32
#include "SafeWindows.hpp"
#elif defined(__linux__)
#include <iconv.h>
#endif

#include "Conf/Conf.h"

MAA_NS_BEGIN

template <typename _ = void>
inline std::string ansi_to_utf8(std::string_view ansi_str)
{
#ifdef _WIN32
    const char* src_str = ansi_str.data();
    const int byte_len = static_cast<int>(ansi_str.length() * sizeof(char));
    int len = MultiByteToWideChar(CP_ACP, 0, src_str, byte_len, nullptr, 0);
    const std::size_t wstr_length = static_cast<std::size_t>(len) + 1U;
    auto wstr = new wchar_t[wstr_length];
    memset(wstr, 0, sizeof(wstr[0]) * wstr_length);
    MultiByteToWideChar(CP_ACP, 0, src_str, byte_len, wstr, len);

    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    const std::size_t str_length = static_cast<std::size_t>(len) + 1;
    auto str = new char[str_length];
    memset(str, 0, sizeof(str[0]) * str_length);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, nullptr, nullptr);
    std::string strTemp = str;

    delete[] wstr;
    wstr = nullptr;
    delete[] str;
    str = nullptr;

    return strTemp;
#elif defined(__linux__)
    iconv_t conv = ::iconv_open("utf-8", "gbk");
    if (conv == (iconv_t)-1) {
        // error
        return std::string(ansi_str);
    }
    // NOTE: 对 std::string_view 使用 strlen 结果可能非预期
    // const char* src_str = ansi_str.data();
    // size_t src_len = ::strlen(src_str) + 1;
    
    const char* from = utf8_str.data();
    size_t len = utf8_str.length();
    size_t dst_len = src_len * 2; // ensure sufficient space

    std::unique_ptr<char[], std::default_delete<char[]>> utf8 = std::make_unique<char[]>(dst_len);
    char* dst = utf8.get();

    char* in_buf = const_cast<char*>(src_str);
    char* out_buf = dst;
    auto res = ::iconv(conv, &in_buf, &src_len, &out_buf, &dst_len);
    if (res == (decltype(res))-1) {
        // error
        ::iconv_close(conv);
        return std::string(ansi_str);
    }

    ::iconv_close(conv);

    return dst;
#else
    return std::string(ansi_str);
#endif
}

template <typename _ = void>
inline std::string utf8_to_ansi(std::string_view utf8_str)
{
#ifdef _WIN32
    const char* src_str = utf8_str.data();
    const int byte_len = static_cast<int>(utf8_str.length() * sizeof(char));
    int len = MultiByteToWideChar(CP_UTF8, 0, src_str, byte_len, nullptr, 0);
    const std::size_t wsz_ansi_length = static_cast<std::size_t>(len) + 1U;
    auto wsz_ansi = new wchar_t[wsz_ansi_length];
    memset(wsz_ansi, 0, sizeof(wsz_ansi[0]) * wsz_ansi_length);
    MultiByteToWideChar(CP_UTF8, 0, src_str, byte_len, wsz_ansi, len);

    len = WideCharToMultiByte(CP_ACP, 0, wsz_ansi, -1, nullptr, 0, nullptr, nullptr);
    const std::size_t sz_ansi_length = static_cast<std::size_t>(len) + 1;
    auto sz_ansi = new char[sz_ansi_length];
    memset(sz_ansi, 0, sizeof(sz_ansi[0]) * sz_ansi_length);
    WideCharToMultiByte(CP_ACP, 0, wsz_ansi, -1, sz_ansi, len, nullptr, nullptr);
    std::string strTemp(sz_ansi);

    delete[] wsz_ansi;
    // wsz_ansi = nullptr; // 无效代码
    delete[] sz_ansi;
    // sz_ansi = nullptr; // 无效代码

    return strTemp;
#elif defined(__linux__)
    iconv_t conv = ::iconv_open("gbk", "utf-8");
    if (conv == (iconv_t)-1) {
        // error
        return std::string(utf8_str);
    }
    // NOTE: 对 std::string_view 使用 strlen 结果可能非预期
    // const char* src_str = utf8_str.data();
    // size_t src_len = ::strlen(src_str) + 1;
    const char* from = utf8_str.data();
    size_t src_len = utf8_str.length();
    size_t dst_len = src_len * 2;

    std::unique_ptr<char[], std::default_delete<char[]>> ansi = std::make_unique<char[]>(dst_len);
    char* dst = ansi.get();

    char* in_buf = const_cast<char*>(src_str);
    char* out_buf = dst;
    auto res = ::iconv(conv, &in_buf, &src_len, &out_buf, &dst_len);
    if (res == (decltype(res))-1) {
        // error
        ::iconv_close(conv);
        return std::string(utf8_str);
    }

    ::iconv_close(conv);

    return dst;
#else
    return std::string(utf8_str);
#endif
}

template <typename _ = void>
inline std::string utf8_to_unicode_escape(std::string_view utf8_str)
{
#ifdef _WIN32
    // FIXME: 由于历史原因，在 Win32 平台下的 wchar_t 采用 UTF-16 编码，和 Linux 等平台使用 UCS-4 不一致，这段代码很可能有问题
    const char* src_str = utf8_str.data();
    int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, nullptr, 0);
    const std::size_t wstr_length = static_cast<std::size_t>(len) + 1U;
    auto wstr = new wchar_t[wstr_length];
    memset(wstr, 0, sizeof(wstr[0]) * wstr_length);
    MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wstr, len);

    std::string unicode_escape_str;
    unicode_escape_str.reserve(len * 6);
    for (const wchar_t* pchr = wstr; *pchr; ++pchr) {
        const wchar_t& chr = *pchr;
        if (chr > 255) {
            unicode_escape_str += MAA_FMT::format("\\u{:04x}", (std::uint32_t)chr);
        }
        else {
            unicode_escape_str.push_back(chr & 255);
        }
    }

    delete[] wstr;
    wstr = nullptr;

    return unicode_escape_str;
#elif defined(__linux__)
    auto locale = setlocale(LC_ALL, "");

    // const char* from = utf8_str.data();
    // size_t len = strlen(from) + 1;

    // NOTE: string_view 末尾不一定是 \0，所以取 data() 后 strlen 会得到非预期结果

    const char* from = utf8_str.data();
    size_t len = utf8_str.length();

    std::unique_ptr<wchar_t[], std::default_delete<wchar_t[]>> to = std::make_unique<wchar_t[]>(len);

    mbstowcs(to.get(), from, len);

    setlocale(LC_ALL, locale); // TODO: 只设置一次 LC_ALL

    std::string unicode_escape_str;
    unicode_escape_str.reserve(len * 6);
    for (const wchar_t* pchr = to.get(); *pchr; ++pchr) {
        const wchar_t& chr = *pchr;
        if (chr > 255) {
            unicode_escape_str += MAA_FMT::format("\\u{:04x}", (std::uint32_t)chr);
        }
        else {
            unicode_escape_str.push_back(chr & 255);
        }
    }

    return unicode_escape_str;
#elif sizeof(wchar_t) == 4
    // fallback: 当 wchar_t 采用 UCS-4 编码时，wchar_t 的对应值即为 Unicode 码
    const std::codecvt<wchar_t, char, std::mbstate_t>& cvt =
        std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(
            std::locale("C.UTF-8")); // FIXME: 应当在程序开头设置 locale，此处则应使用 std::locale()...
    std::mbstate_t mb {};
    std::wstring to(utf8_str.length(), L'\0'); // t.size() <= utf8_str.size()
    const char* from_next;
    wchar_t* to_next;
    std::codecvt_base::result result =
        cvt.in(mb, &utf8_str[0], &utf8_str[utf8_str.length()], from_next, &to[0], &to[to.length()], to_next);
    if (result != std::codecvt_base::ok) {
        return std::string(utf8_str); // error
    }
    to.resize(to_next - &to[0]); // 或许这一句是不必要的...但是嘛，优雅第一
    std::string unicode_escape_str;
    unicode_escape_str.reserve(len * 6);
    for (const wchar_t& chr : to) {
        if (chr > 255) {
            unicode_escape_str += MAA_FMT::format("\\u{:04x}", (std::uint32_t)chr);
        }
        else {
            unicode_escape_str.push_back(chr & 255);
        }
    }
#else
    return std::string(utf8_str);
#endif
}

inline std::string utf8_to_crt(std::string_view utf8_str)
{
#ifdef _WIN32
    return utf8_to_ansi(utf8_str);
#else
    return std::string(utf8_str);
#endif
}

inline std::string load_file_without_bom(const std::filesystem::path& path)
{
    std::ifstream ifs(path, std::ios::in);
    if (!ifs.is_open()) {
        return {};
    }
    std::stringstream iss;
    iss << ifs.rdbuf();
    ifs.close();
    std::string str = iss.str();

    if (str.starts_with("\xEF\xBB\xBF")) {
        str.assign(str.begin() + 3, str.end());
    }
    return str;
}

MAA_NS_END
