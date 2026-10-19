// HID 注入这条路的字节级自检。
//
// 为什么要有：`send` 是只发不收的，编码错了设备**不会**告诉你，现场只是"屏幕
// 没反应"。这类错误没有比"钉住一个可用客户端的真实字节"更硬的判据了——下面
// 那段 284 字节的基准是从一个确认能在设备上画出东西的客户端抓下来的原样字节。
#include <cstdio>
#include <limits>
#include <span>
#include <string>
#include <vector>

#include "hid/Hid.h"
#include "xpc/XpcValue.h"

namespace
{

int Failures = 0;

void check(bool ok, const std::string& what)
{
    std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what.c_str());
    if (!ok) {
        ++Failures;
    }
}

const char* kHex = "0123456789abcdef";

std::string hex(std::span<const uint8_t> v)
{
    std::string s;
    for (uint8_t b : v) {
        s += kHex[b >> 4];
        s += kHex[b & 0xF];
    }
    return s;
}

std::vector<uint8_t> unhex(const std::string& s)
{
    auto nibble = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') {
            return static_cast<uint8_t>(c - '0');
        }
        return static_cast<uint8_t>(c - 'a' + 10);
    };
    std::vector<uint8_t> v;
    for (std::size_t i = 0; i + 1 < s.size(); i += 2) {
        v.push_back(static_cast<uint8_t>(nibble(s[i]) << 4 | nibble(s[i + 1])));
    }
    return v;
}

using namespace maa::ios::xpc;

/// 报告本体：09 01 05 <state> <x LE16> <y LE16> <32 个 0> <02 00 00 00>
/// <时间戳 6 字节 LE> <8 个 0>。
void test_touchscreen_report()
{
    std::printf("\n== mainTouchscreen 报告布局 ==\n");
    const auto contact = maa::ios::hid::touchscreen_report(maa::ios::hid::kStateContact, 0x7FFF, 0x7FFF, 0x3FC14C1Dull);
    const auto release = maa::ios::hid::touchscreen_report(maa::ios::hid::kStateRelease, 0x7FFF, 0x7FFF, 0x3FC14C1Dull);
    check(contact.size() == 58, "长度 58: " + std::to_string(contact.size()));
    check(hex(contact) ==
              "090105c2ff7fff7f"
              "0000000000000000000000000000000000000000000000000000000000000000"
              "02000000"
              "1d4cc13f0000"
              "0000000000000000",
          "接触报告的字节与基准一致");
    check(hex(release).substr(6, 2) == "02", "抬起只改状态字节");
    // 时间戳是 6 字节而不是 8：多写两字节会把后面的保留区挤掉。
    check(contact[44] == 0x1d && contact[49] == 0x00 && contact[50] == 0x00, "时间戳占 44..49，其后 8 字节保留");
}

void test_normalize()
{
    std::printf("\n== 坐标归一化 ==\n");
    check(maa::ios::hid::normalize(0.0) == 0, "0 -> 0");
    check(maa::ios::hid::normalize(1.0) == 0xFFFF, "1 -> 65535");
    check(maa::ios::hid::normalize(0.5) == 32768 || maa::ios::hid::normalize(0.5) == 32767, "0.5 -> 屏幕正中");
    check(maa::ios::hid::normalize(-3.0) == 0, "负数夹到 0，不取模");
    check(maa::ios::hid::normalize(9.0) == 0xFFFF, "超过 1 夹到 65535");
    check(maa::ios::hid::normalize(std::numeric_limits<double>::quiet_NaN()) == 0, "NaN 落到 0 而不是变成随机 UInt16");
}

/// 一个可用客户端为"在 (32767,32767) 按下一刻"发出的整条消息。
void test_send_message_golden()
{
    std::printf("\n== send 请求整条消息对基准 ==\n");
    const std::string kGolden =
        "920bb02901010000040100000000000001000000000000004237134205000000"
        "00f00000f400000003000000666561747572654964656e746966696572000000"
        "0090000038000000636f6d2e6170706c652e636f72656465766963652e666561"
        "747572652e72656d6f74652e756e6976657273616c6869647365727669636500"
        "6d6573736167655479706500009000000800000052657175657374007061796c"
        "6f61640000f00000700000000100000073656e640000000000f000005c000000"
        "020000005f300000008000003a000000090105c2ff7fff7f0000000000000000"
        "000000000000000000000000000000000000000000000000020000001d4cc13f"
        "0000000000000000000000005f310000004000000101000000000000";

    Value args = make_dict();
    dict_set(args, "_0", make_data(maa::ios::hid::touchscreen_report(maa::ios::hid::kStateContact, 32767, 32767, 0x3FC14C1Dull)));
    dict_set(args, "_1", make_uint64(257));
    Value payload = make_dict();
    dict_set(payload, "send", std::move(args));

    Value msg = make_dict();
    dict_set(msg, "featureIdentifier", make_string("com.apple.coredevice.feature.remote.universalhidservice"));
    dict_set(msg, "messageType", make_string("Request"));
    dict_set(msg, "payload", std::move(payload));

    const auto got = encode_message(kFlagAlwaysSet | kFlagDataPresent, 1, &msg);
    if (hex(got) != kGolden) {
        std::printf("        实际 %s\n", hex(got).c_str());
    }
    check(hex(got) == kGolden, "284 字节逐字节相同（含 Data 不补零这条规则）");
    check(got.size() == unhex(kGolden).size(), "长度 284");
}

/// 键盘报告与 ASCII 翻译。usage 号是 USB-IF HID Usage Tables page 0x07 的公开值。
void test_keyboard()
{
    std::printf("\n== 虚拟键盘报告 ==\n");
    const auto a = maa::ios::hid::keyboard_report({ maa::ios::hid::key::kA }, 0x3FC14C1Dull);
    check(a.size() == 39, "长度 39: " + std::to_string(a.size()));
    check(a[0] == 0x01, "报告号 0x01");
    // usage 4 -> 字节 1 + 4/8 = 1 的第 4 位
    check(a[1] == 0x10, "a 的位图落点正确");
    check(hex(std::span<const uint8_t>(a.data() + 31, 6)) == "1d4cc13f0000", "时间戳占 31..36");
    const auto none = maa::ios::hid::keyboard_report({ }, 0x3FC14C1Dull);
    check(none[1] == 0x00, "空集合就是全部松开");
    // 240 位装不下的 usage 必须被丢掉而不是写坏别的字节：拿它和"空集合、同一
    // 时间戳"比，时间戳不一样就比不出位图了。
    const auto overflow = maa::ios::hid::keyboard_report({ 500 }, 0x3FC14C1Dull);
    check(overflow == none, "超出 240 位的 usage 被忽略");

    std::printf("\n== ASCII -> usage 序列 ==\n");
    const auto ab = maa::ios::hid::text_reports("ab");
    check(ab.size() == 4, "两个字母 = 按下/松开 各两组: " + std::to_string(ab.size()));
    check(ab.size() == 4 && ab[0].size() == 1 && ab[0][0] == maa::ios::hid::key::kA, "第一个是 a 按下");
    check(ab.size() == 4 && ab[1].empty(), "第二个是松开");
    check(ab.size() == 4 && ab[2].size() == 1 && ab[2][0] == maa::ios::hid::key::kA + 1, "第三个是 b 按下");

    // 带 Shift 的字符是**三条**报告：先按下 Shift，再发 Shift+键，最后全松。
    // 合成一条 {Shift, 键} 实测只会出 "1" 而不是 "!"。
    const auto bang = maa::ios::hid::text_reports("!");
    check(bang.size() == 3, "! 展开成 3 条报告: " + std::to_string(bang.size()));
    if (bang.size() == 3) {
        check(bang[0].size() == 1 && bang[0][0] == maa::ios::hid::key::kShiftLeft, "第一条只有 Shift");
        check(
            bang[1].size() == 2 && bang[1][0] == maa::ios::hid::key::kShiftLeft && bang[1][1] == maa::ios::hid::key::k1,
            "第二条是 Shift + 1");
        check(bang[2].empty(), "第三条全松");
    }

    const auto junk = maa::ios::hid::text_reports("\u00e9\x01");
    check(junk.empty(), "认不出的字符跳过而不是抛错");
}

} // namespace

int main()
{
    test_keyboard();
    test_touchscreen_report();
    test_normalize();
    test_send_message_golden();
    std::printf("\n%s (失败 %d 项)\n", Failures == 0 ? "全部通过" : "存在失败", Failures);
    return Failures == 0 ? 0 : 1;
}
