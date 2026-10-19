// plist 子集实现的自检。
//
// 输入样本按 usbmuxd 真实响应的结构构造，但序列号是假的——真实抓包含设备
// UDID，不入库。
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "plist/Plist.h"

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

constexpr const char* kAttached = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>DeviceID</key>
	<integer>7</integer>
	<key>MessageType</key>
	<string>Attached</string>
	<key>Properties</key>
	<dict>
		<key>ConnectionSpeed</key>
		<integer>480000000</integer>
		<key>ConnectionType</key>
		<string>USB</string>
		<key>DeviceID</key>
		<integer>7</integer>
		<key>LocationID</key>
		<integer>1048576</integer>
		<key>ProductID</key>
		<integer>4776</integer>
		<key>SerialNumber</key>
		<string>00000000-0000000000000000</string>
		<key>USBSerialNumber</key>
		<string>000000000000000000000000</string>
	</dict>
</dict>
</plist>
)";

constexpr const char* kResult = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>MessageType</key>
	<string>Result</string>
	<key>Number</key>
	<integer>0</integer>
</dict>
</plist>
)";

void test_real_shapes()
{
    std::printf("== usbmuxd 真实结构 ==\n");
    std::string err;
    auto v = maa::ios::plist::parse(kAttached, &err);
    check(v.has_value(), "解析 Attached 事件: " + err);
    if (!v) {
        return;
    }
    check(v->is_dict(), "根是 dict");
    const auto* props = v->find("Properties");
    check(props != nullptr && props->is_dict(), "Properties 是 dict");
    if (props) {
        check(props->find("SerialNumber")->as_string_or("?") == "00000000-0000000000000000", "SerialNumber 字符串正确");
        check(props->find("ConnectionType")->as_string_or("?") == "USB", "ConnectionType=USB");
        check(props->find("ConnectionSpeed")->as_int_or(0) == 480'000'000, "ConnectionSpeed 整数正确");
        check(props->find("ProductID")->as_int_or(0) == 4776, "ProductID 整数正确");
    }
    check(v->find("DeviceID")->as_int_or(0) == 7, "顶层 DeviceID=7（制表符缩进被容忍）");
    check(v->find("Nope") == nullptr, "不存在的键返回 nullptr");

    auto r = maa::ios::plist::parse(kResult);
    check(r.has_value() && r->find("Number")->as_int_or(-1) == 0, "解析 Result 响应");
}

void test_round_trip()
{
    std::printf("\n== 往返 ==\n");
    using namespace maa::ios::plist;
    Value d = Value::Dict();
    d.set("S", Value::Str("hello & <world> \"x\" 'y'"));
    d.set("I", Value::Int(-42));
    d.set("B", Value::Bool(true));
    d.set("F", Value::Bool(false));
    std::vector<uint8_t> bin;
    for (int i = 0; i < 256; ++i) {
        bin.push_back(static_cast<uint8_t>(i));
    }
    d.set("D", Value::OfData(bin));
    Value a = Value::Array();
    a.push(Value::Int(1));
    a.push(Value::Str("two"));
    Value nested = Value::Dict();
    nested.set("deep", Value::Bool(false));
    a.push(std::move(nested));
    d.set("A", std::move(a));
    d.set("EmptyDict", Value::Dict());
    d.set("EmptyArr", Value::Array());

    const std::string xml = write(d);
    auto back = parse(xml);
    check(back.has_value(), "写出的 XML 能重新解析");
    if (!back) {
        return;
    }
    check(back->find("S")->as_string_or("") == "hello & <world> \"x\" 'y'", "字符串里的 &<>\"' 往返无损");
    check(back->find("I")->as_int_or(0) == -42, "负整数往返");
    check(back->find("B")->as_bool_or(false) == true, "true 往返");
    check(back->find("F")->as_bool_or(true) == false, "false 往返");
    check(back->find("D")->data == bin, "0x00-0xFF 全字节 base64 往返");
    const auto* arr = back->find("A");
    check(arr && arr->array.size() == 3, "array 三个元素");
    check(arr && arr->array[1].as_string_or("") == "two", "array[1] 字符串");
    check(arr && arr->array[2].find("deep")->as_bool_or(true) == false, "嵌套 dict 内的 bool");
    check(back->find("EmptyDict") && back->find("EmptyDict")->is_dict() && back->find("EmptyDict")->keys.empty(), "空 dict 往返");
    check(back->find("EmptyArr") && back->find("EmptyArr")->is_array() && back->find("EmptyArr")->array.empty(), "空 array 往返");
}

void test_edge_cases()
{
    std::printf("\n== 边界与拒绝 ==\n");
    std::string err;

    auto e1 = maa::ios::plist::parse("<plist><dict><key>a&amp;b&lt;&gt;&#65;&#x42;</key><string>x</string></dict></plist>");
    check(e1.has_value() && e1->keys.size() == 1 && e1->keys[0] == "a&b<>AB", "实体与数字字符引用解码（含 10 进制与 16 进制）");

    auto e2 = maa::ios::plist::parse("<plist><string>  keep  me\n  </string></plist>");
    check(e2.has_value() && e2->as_string_or("") == "  keep  me\n  ", "<string> 内部空白不被裁剪");

    auto e3 = maa::ios::plist::parse("<plist><integer> 0x1F </integer></plist>");
    check(e3.has_value() && e3->as_int_or(0) == 31, "integer 支持 0x 形式与空白");
    for (const auto* number : { "9223372036854775808", "18446744073709551615", "0xffffffffffffffff" }) {
        auto value = maa::ios::plist::parse(std::string("<plist><integer>") + number + "</integer></plist>");
        auto back = value ? maa::ios::plist::parse(maa::ios::plist::write(*value)) : std::nullopt;
        check(value && value->is_uint() && back && back->as_uint_or() == value->as_uint_or(), "全宽无符号整数往返无损");
    }

    auto e4 = maa::ios::plist::parse("<plist><dict><key>k</key><string>v</string></dict></plist");
    check(!e4.has_value(), "缺少 </plist> 被拒绝");

    auto e5 = maa::ios::plist::parse("<plist><dict><key>k</key><string>v</dict></plist>");
    check(!e5.has_value(), "闭合标签不匹配被拒绝");

    auto e6 = maa::ios::plist::parse("<plist><bogus>1</bogus></plist>");
    check(!e6.has_value(), "不支持的元素类型被拒绝");

    std::string deep = "<plist>";
    for (int i = 0; i < 200; ++i) {
        deep += "<array>";
    }
    deep += "<true/>";
    for (int i = 0; i < 200; ++i) {
        deep += "</array>";
    }
    deep += "</plist>";
    check(!maa::ios::plist::parse(deep).has_value(), "超深嵌套被深度上限拦下（不打爆栈）");

    auto e7 = maa::ios::plist::parse("<plist><data>@@@not-base64@@@</data></plist>");
    check(!e7.has_value(), "非法 base64 被拒绝");

    auto e8 = maa::ios::plist::parse("<plist><array/><dict/></plist>");
    check(!e8.has_value(), "根元素只能是单个值");

    auto e9 = maa::ios::plist::parse("<?xml version=\"1.0\"?>\n<!-- 前导注释 -->\n<plist><true/></plist>");
    check(e9.has_value() && e9->as_bool_or(false), "XML 声明与前导注释被跳过");

    auto e10 = maa::ios::plist::parse("<plist><real>1.5</real></plist>");
    check(e10.has_value() && e10->kind == maa::ios::plist::Kind::Real && e10->real == 1.5, "real 类型解析");
    auto unicode = maa::ios::plist::parse("<plist><string><![CDATA[中文<&]]><!-- c -->尾</string></plist>");
    check(unicode && unicode->string == "中文<&尾", "CDATA、Unicode 和注释组成标量文本");
    for (const auto* invalid : { "<plist><true/></plist><plist><false/></plist>",
                                 "<plist><true/></plist>garbage",
                                 "<plist><dict>garbage</dict></plist>",
                                 "<plist><dict><key>x</key></dict></plist>",
                                 "<plist><string><true/></string></plist>",
                                 "<plist><integer>42oops</integer></plist>",
                                 "<plist><integer>18446744073709551616</integer></plist>",
                                 "<plist><real>nan</real></plist>",
                                 "<plist><real>inf</real></plist>",
                                 "<plist><true>no</true></plist>" }) {
        check(!maa::ios::plist::parse(invalid, &err) && !err.empty(), std::string("拒绝非法 plist 结构或数值: ") + invalid);
    }
    std::string nul = "<plist><string>prefix";
    nul += '\0';
    nul += "suffix</string></plist>";
    check(!maa::ios::plist::parse(nul), "拒绝 NUL 而非截断输入");
    bool rejected_nul = false;
    try {
        (void)maa::ios::plist::write(maa::ios::plist::Value::Str(std::string("a\0b", 3)));
    }
    catch (const std::invalid_argument&) {
        rejected_nul = true;
    }
    check(rejected_nul, "序列化拒绝 NUL 而非静默丢弃后续文本");
    check(!maa::ios::plist::parse(std::string((8u << 20) + 1, ' ')), "拒绝超过 8 MiB 的输入");
}

} // namespace

int main()
{
    test_real_shapes();
    test_round_trip();
    test_edge_cases();
    std::printf("\n%s (失败 %d 项)\n", Failures == 0 ? "全部通过" : "存在失败", Failures);
    return Failures == 0 ? 0 : 1;
}
