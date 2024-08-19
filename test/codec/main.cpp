#include "Utils/Codec.h"

#include <cstdint>
#include <iostream>

#ifdef _WIN32
using wchar_uint = uint16_t;
#else
using wchar_uint = uint32_t;
#endif

bool test_to_u16()
{
    std::cout << "Test to_u16" << std::endl;

    std::string u8str = "abc你好😊";

    size_t expectSrcLength = 13;

    if (u8str.length() != expectSrcLength) {
        std::cout << "Src length not match, expect " << expectSrcLength << ", got "
                  << u8str.length() << std::endl;
    }

    std::wstring utfstr = MaaNS::to_u16(u8str);
#ifdef _WIN32
    size_t expectLength = 7;
    uint32_t expectValues[] = { 97, 98, 99, 0x4f60, 0x597d, 0xD83D, 0xDE0A };
#else
    size_t expectLength = 6;
    uint32_t expectValues[] = { 97, 98, 99, 0x4f60, 0x597d, 0x1F60A };
#endif

    if (utfstr.length() != expectLength) {
        std::cout << "Length not match, expect " << expectLength << ", got " << utfstr.length()
                  << std::endl;

        std::cout << "expect ";
        for (auto ch : expectValues) {
            std::cout << std::hex << ch << ' ';
        }
        std::cout << std::endl;

        std::cout << "got    ";
        for (auto ch : utfstr) {
            std::cout << std::hex << static_cast<wchar_uint>(ch) << ' ';
        }
        std::cout << std::endl;

        return false;
    }

    for (size_t i = 0; i < expectLength; i++) {
        if (static_cast<uint32_t>(static_cast<wchar_uint>(utfstr[i])) != expectValues[i]) {
            std::cout << "Character at " << i << " not match, expect " << std::hex
                      << expectValues[i] << ", got " << std::hex << static_cast<uint32_t>(utfstr[i])
                      << std::endl;
            return false;
        }
    }

    return true;
}

bool test_from_u16()
{
    std::cout << "Test from_u16" << std::endl;

    std::wstring utfstr = L"abc你好😊";

#ifdef _WIN32
    size_t expectSrcLength = 7;
#else
    size_t expectSrcLength = 6;
#endif

    if (utfstr.length() != expectSrcLength) {
        std::cout << "Src length not match, expect " << expectSrcLength << ", got "
                  << utfstr.length() << std::endl;
    }

    std::string u8str = MaaNS::from_u16(utfstr);

    size_t expectLength = 13;
    uint32_t expectValues[] = { 0x61, 0x62, 0x63, 0xE4, 0xBD, 0xA0, 0xE5,
                                0xA5, 0xBD, 0xF0, 0x9F, 0x98, 0x8A };

    if (u8str.length() != expectLength) {
        std::cout << "Length not match, expect " << expectLength << ", got " << u8str.length()
                  << std::endl;

        std::cout << "expect ";
        for (auto ch : expectValues) {
            std::cout << std::hex << ch << ' ';
        }
        std::cout << std::endl;

        std::cout << "got    ";
        for (auto ch : u8str) {
            std::cout << std::hex << static_cast<unsigned>(static_cast<uint8_t>(ch)) << ' ';
        }
        std::cout << std::endl;

        return false;
    }

    for (size_t i = 0; i < expectLength; i++) {
        if (static_cast<uint32_t>(static_cast<uint8_t>(u8str[i])) != expectValues[i]) {
            std::cout << "Character at " << i << " not match, expect " << std::hex
                      << expectValues[i] << ", got " << std::hex << static_cast<uint32_t>(u8str[i])
                      << std::endl;
            return false;
        }
    }

    return true;
}

int main()
{
    if (!test_to_u16()) {
        return -1;
    }

    if (!test_from_u16()) {
        return -1;
    }

    return 0;
}
