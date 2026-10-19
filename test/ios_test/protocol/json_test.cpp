// JSON 协议边界自检。隧道握手与后续 RemoteXPC 元数据都靠它。
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

#include "remote/PairingChannel.h"
#include "json/Json.h"

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

constexpr const char* kHandshake = R"({
  "type": "serverHandshakeResponse",
  "serverAddress": "fd78:6049:c266::1",
  "serverRSDPort": 58425,
  "mtu": 16000,
  "clientParameters": { "address": "fd78:6049:c266::2", "mtu": 16000 }
})";

} // namespace

int main()
{
    std::printf("== 隧道握手响应 ==\n");
    std::string err;
    auto v = maa::ios::json::parse(kHandshake, &err);
    check(v.has_value(), "解析握手响应: " + err);
    if (v) {
        check(maa::ios::json::as_int_or(*maa::ios::json::find(*v, "serverRSDPort"), 0) == 58425, "整数端口");
        check(maa::ios::json::as_string_or(*maa::ios::json::find(*v, "serverAddress"), "") == "fd78:6049:c266::1", "IPv6 字符串");
        const auto* cp = maa::ios::json::find(*v, "clientParameters");
        check(
            cp != nullptr && maa::ios::json::as_string_or(*maa::ios::json::find(*cp, "address"), "") == "fd78:6049:c266::2",
            "嵌套 object 取值");
        check(maa::ios::json::find(*v, "Nope") == nullptr, "不存在的键返回 nullptr");
    }

    std::printf("\n== 标量与转义 ==\n");
    auto scalars = maa::ios::json::parse(R"({"a":1,"b":-2,"c":1.5,"d":true,"e":false,"f":null})");
    check(scalars.has_value(), "混合标量 object");
    if (scalars) {
        check(maa::ios::json::as_int_or(*maa::ios::json::find(*scalars, "b"), 0) == -2, "负整数");
        check(maa::ios::json::find(*scalars, "c")->is<double>(), "小数走 Double");
        check(maa::ios::json::find(*scalars, "d")->as_boolean(), "true");
        check(!maa::ios::json::find(*scalars, "e")->as_boolean(), "false");
        check(maa::ios::json::find(*scalars, "f")->is_null(), "null");
    }

    auto esc = maa::ios::json::parse(R"j("q\"b\\n\nuAéВ")j");
    check(esc.has_value() && maa::ios::json::as_string_or(*esc, "") == "q\"b\\n\nuAéВ", "转义与 \\u（含 BMP 与代理对）");

    auto arr = maa::ios::json::parse("[1,2,[3,4],{\"k\":[5]}]");
    check(arr.has_value() && arr->as_array().size() == 4 && (*arr)[2].as_array().size() == 2, "嵌套数组");

    auto empty = maa::ios::json::parse(R"({"a":[],"b":{}})");
    check(empty.has_value() && maa::ios::json::find(*empty, "a")->empty() && maa::ios::json::find(*empty, "b")->empty(), "空数组与空对象");

    // 10 层嵌套远在深度上限之内，必须正常接受。
    auto nested = maa::ios::json::parse("[[[[[[[[[[1]]]]]]]]]]");
    check(nested.has_value(), "10 层嵌套应被正常接受");

    std::printf("\n== 必须拒绝的畸形输入 ==\n");
    check(!maa::ios::json::parse("{").has_value(), "截断的 object");
    check(!maa::ios::json::parse(R"({"a"})").has_value(), "缺值的键");
    check(!maa::ios::json::parse(R"({"a":1,})").has_value(), "尾随逗号");
    check(!maa::ios::json::parse(R"([1] x)").has_value(), "尾部多余内容");
    check(!maa::ios::json::parse(R"j("unterminated)j").has_value(), "未闭合字符串");

    std::string deep = "[";
    for (int i = 0; i < 200; ++i) {
        deep += "[";
    }
    deep += "1";
    for (int i = 0; i < 201; ++i) {
        deep += "]";
    }
    check(!maa::ios::json::parse(deep).has_value(), "超深嵌套被深度上限拦下");

    for (const auto* bad : { "01", "+1", "1e", "1-2", "1.2.3", R"("\uD800")" }) {
        check(!maa::ios::json::parse(bad), std::string("非法 JSON 必须拒绝: ") + bad);
    }
    check(!maa::ios::json::parse(std::string("\"line\nfeed\"")), "字符串内原始换行必须拒绝");
    check(!maa::ios::json::parse(std::string("\"") + static_cast<char>(0xff) + "\""), "非法 UTF-8 必须拒绝");
    check(!maa::ios::json::parse(std::string((4u << 20) + 1, ' ')), "输入大小上限");
    const std::string depth64 = std::string(64, '[') + "0" + std::string(64, ']');
    check(maa::ios::json::parse(depth64).has_value(), "64 层边界应接受");
    check(!maa::ios::json::parse("[" + depth64 + "]"), "65 层边界应拒绝");
    const auto wide = maa::ios::json::parse("18446744073709551615");
    check(wide && maa::ios::json::as_int_or(*wide, -1) == -1, "uint64 溢出不能变成负整数");
    check(maa::ios::json::as_int_or(maa::ios::json::Value(0x1p63), -1) == -1, "浮点整数越界返回 fallback");
    check(maa::ios::json::find(::json::array { }, "x") == nullptr, "非对象查键不抛异常");

    check(!maa::ios::json::parse("[1e999]"), "reject overflowing floating-point input");
    check(!maa::ios::json::parse("1,2"), "reject multiple root values");
    check(maa::ios::json::parse("true")->as_boolean(), "boolean root");
    check(maa::ios::json::parse("null")->is_null(), "null root");
    check(maa::ios::json::parse("\xef\xbb\xbf{}", &err).has_value(), "UTF-8 BOM");
    check(maa::ios::json::parse(R"("\uD83D\uDE42")")->as_string() == "\xf0\x9f\x99\x82", "Unicode surrogate pair");
    check(!maa::ios::json::parse(std::string("\"") + "\xc0\xaf" + "\""), "reject overlong UTF-8");
    check(!maa::ios::json::parse(std::string("\"") + "\xed\xa0\x80" + "\""), "reject UTF-8 surrogate code point");
    check(
        maa::ios::json::write(maa::ios::json::Value(std::string(1, static_cast<char>(0xff))), true) == "\"\xef\xbf\xbd\"",
        "management output replaces invalid UTF-8");

    using namespace maa::ios;
    for (double number : { 1.0, -0.0, 1.2345678901234567, 1e-12, std::numeric_limits<double>::max() }) {
        auto encoded = remote::xpc_to_json(xpc::make_double(number), err);
        const auto decoded = encoded ? remote::json_to_xpc(*encoded, err) : std::nullopt;
        check(
            decoded && decoded->type == xpc::Type::Double && decoded->real == number && std::signbit(decoded->real) == std::signbit(number),
            "XPC double retains precision and type");
    }
    auto sequence = maa::ios::json::parse(R"({"sequenceNumber":18446744073709551615})");
    auto sequence_xpc = remote::json_to_xpc(*sequence, err);
    check(
        sequence_xpc && sequence_xpc->at("sequenceNumber").type == xpc::Type::UInt64
            && sequence_xpc->at("sequenceNumber").uint64 == UINT64_MAX,
        "sequence number retains uint64 range");
    auto oversized_integer = remote::json_to_xpc(*wide, err);
    check(!oversized_integer, "non-sequence integer cannot exceed XPC int64 range");
    auto binary = maa::ios::json::parse(R"({"message":{"streamEncrypted":{"_0":"AQI="}}})");
    const auto binary_xpc = remote::json_to_xpc(*binary, err);
    check(
        binary_xpc && binary_xpc->at("message").at("streamEncrypted").at("_0").type == xpc::Type::Data
            && binary_xpc->at("message").at("streamEncrypted").at("_0").data == wifi::Bytes({ 1, 2 }),
        "pairing ciphertext remains XPC data");

    std::printf("\n== 往返 ==\n");
    auto src = maa::ios::json::parse(kHandshake);
    check(src.has_value(), "源可解析");
    if (src) {
        auto again = maa::ios::json::parse(maa::ios::json::write(*src));
        check(again.has_value() && maa::ios::json::write(*again) == maa::ios::json::write(*src), "write -> parse -> write 稳定");
    }

    std::printf("\n%s (失败 %d 项)\n", Failures == 0 ? "全部通过" : "存在失败", Failures);
    return Failures == 0 ? 0 : 1;
}
