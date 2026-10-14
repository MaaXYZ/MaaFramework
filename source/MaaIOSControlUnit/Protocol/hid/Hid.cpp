#include "Hid.h"
#include "i18n/Translation.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

#include "remote/Rsd.h"
#include "xpc/XpcValue.h"

namespace maa::ios::hid
{
namespace
{

constexpr std::string_view kServiceName = "com.apple.coredevice.hid.universalhidservice";
constexpr std::string_view kFeature = "com.apple.coredevice.feature.remote.universalhidservice";
constexpr std::string_view kIndigoServiceName = "com.apple.coredevice.hid.indigo";
constexpr std::string_view kButtonFeature = "com.apple.coredevice.feature.remote.hid.button";
constexpr std::size_t kTouchscreenReportLen = 58;
constexpr std::size_t kKeyboardReportLen = 39;

/// HID 请求直接使用 messageType、payload 和 featureIdentifier 字段。
/// 不添加 CoreDevice.input、actionIdentifier 等通用 feature 请求包装；
/// 已有设备使用此布局，不能改用 core_device_request() 代替。
xpc::Value request(std::string_view feature_identifier, std::string_view message_type, xpc::Value payload)
{
    xpc::Value msg = xpc::make_dict();
    xpc::dict_set(msg, "featureIdentifier", xpc::make_string(std::string(feature_identifier)));
    xpc::dict_set(msg, "messageType", xpc::make_string(std::string(message_type)));
    xpc::dict_set(msg, "payload", std::move(payload));
    return msg;
}

/// universalhidservice 使用 Request 消息，payload 为 {动作名: 参数} 字典。
xpc::Value universal_request(std::string_view key, xpc::Value payload)
{
    xpc::Value body = xpc::make_dict();
    xpc::dict_set(body, std::string(key), std::move(payload));
    return request(kFeature, "Request", std::move(body));
}

} // namespace

uint64_t report_timestamp()
{
    // 起点只初始化一次；使用单调时钟表达报告间隔，传输字段保留低 48 位。
    static const auto epoch = std::chrono::steady_clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - epoch).count();
    return static_cast<uint64_t>(ns) & 0xFFFFFFFFFFFFull;
}

std::vector<uint8_t> touchscreen_report(uint8_t state, uint16_t x, uint16_t y, uint64_t timestamp)
{
    std::vector<uint8_t> r(kTouchscreenReportLen, 0);
    r[0] = 0x09;
    r[1] = 0x01;
    r[2] = 0x05;
    r[3] = state;
    r[4] = static_cast<uint8_t>(x & 0xFF);
    r[5] = static_cast<uint8_t>(x >> 8);
    r[6] = static_cast<uint8_t>(y & 0xFF);
    r[7] = static_cast<uint8_t>(y >> 8);
    r[40] = 0x02;
    for (int i = 0; i < 6; ++i) {
        r[44 + static_cast<std::size_t>(i)] = static_cast<uint8_t>(timestamp >> (8 * i));
    }
    return r;
}

std::vector<uint8_t> keyboard_report(const std::vector<uint16_t>& usages, uint64_t timestamp)
{
    std::vector<uint8_t> r(kKeyboardReportLen, 0);
    r[0] = 0x01;
    for (const uint16_t u : usages) {
        // 位图对应 usage 0..239；忽略范围外的值，避免写入时间戳或保留区。
        if (u < 240) {
            r[1 + u / 8] = static_cast<uint8_t>(r[1 + u / 8] | (uint8_t { 1 } << (u % 8)));
        }
    }
    for (int i = 0; i < 6; ++i) {
        r[31 + static_cast<std::size_t>(i)] = static_cast<uint8_t>(timestamp >> (8 * i));
    }
    return r;
}

uint16_t normalize(double v)
{
    if (!(v > 0.0)) { // 非正值及 NaN 均映射到 0。
        return 0;
    }
    if (v >= 1.0) {
        return 0xFFFF;
    }
    return static_cast<uint16_t>(std::lround(v * 65535.0));
}

std::unique_ptr<Service> Service::open(maa::ios::remote::Device& device, std::string& err, bool verbose)
{
    if (!device.rsd().has_service(kServiceName)) {
        err = MAA_IOS_TR("RSD directory missing ") + std::string(kServiceName);
        return nullptr;
    }
    auto conn = device.connect(kServiceName, err, verbose);
    if (conn == nullptr) {
        return nullptr;
    }
    return std::unique_ptr<Service>(new Service(std::move(conn)));
}

bool Service::raw_connected_services(xpc::Value& reply, std::string& err)
{
    return conn_->call(universal_request("connectedServices", xpc::make_dict()), reply, 10000, err);
}

bool Service::surfaces(std::vector<Surface>& out, std::string& err)
{
    xpc::Value reply;
    if (!raw_connected_services(reply, err)) {
        return false;
    }
    const auto* list = reply.find("connectedServices");
    if (list == nullptr) {
        // 必需字段缺失时保留原始回复供诊断，不推断其它字段为服务目录。
        err = MAA_IOS_TR("connectedServices response missing connectedServices: ") + xpc::describe(reply);
        return false;
    }
    const auto each = [&](const xpc::Value& v) {
        Surface s;
        s.service_id = static_cast<uint64_t>(v.at("_ServiceID").as_int_or(0));
        s.name = v.at("Product").as_string_or(v.at("ServiceName").as_string_or(""));
        if (s.service_id != 0) {
            out.push_back(std::move(s));
        }
    };
    if (list->is_array()) {
        for (const auto& v : list->array) {
            each(v);
        }
    }
    else if (list->is_dict()) {
        for (const auto& e : list->dict) {
            each(e.value);
        }
    }
    return true;
}

bool Service::send_report(uint64_t service_id, std::span<const uint8_t> report, std::string& err, xpc::Value* reply)
{
    // send 参数按位置编号：_0 是报告 Data，_1 是目标面的 UInt64 标识。
    xpc::Value args = xpc::make_dict();
    xpc::dict_set(args, "_0", xpc::make_data(std::vector<uint8_t>(report.begin(), report.end())));
    xpc::dict_set(args, "_1", xpc::make_uint64(service_id));
    const auto msg = universal_request("send", std::move(args));
    if (reply == nullptr) {
        return conn_->send_only(msg, err);
    }
    return conn_->call(msg, *reply, 5000, err);
}

bool Service::touch(uint64_t service_id, double x, double y, bool down, std::string& err)
{
    return send_report(service_id, touchscreen_report(down ? kStateContact : kStateRelease, normalize(x), normalize(y)), err);
}

bool Service::tap(double x, double y, int hold_ms, std::string& err)
{
    const uint16_t nx = normalize(x);
    const uint16_t ny = normalize(y);
    if (!send_report(kSurfaceMainTouchscreen, touchscreen_report(kStateContact, nx, ny), err)) {
        return false;
    }
    if (hold_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
    }
    return send_report(kSurfaceMainTouchscreen, touchscreen_report(kStateRelease, nx, ny), err);
}

bool Service::stroke(const std::vector<std::pair<double, double>>& points, int step_ms, std::string& err)
{
    if (points.empty()) {
        err = MAA_IOS_TR("Stroke requires at least one point");
        return false;
    }
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto [x, y] = points[i];
        const bool last = i + 1 == points.size();
        if (!send_report(
                kSurfaceMainTouchscreen,
                touchscreen_report(last ? kStateRelease : kStateContact, normalize(x), normalize(y)),
                err)) {
            return false;
        }
        if (!last && step_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(step_ms));
        }
    }
    return true;
}

bool Service::type(uint64_t surface, const std::vector<uint16_t>& usages, int hold_ms, std::string& err)
{
    if (!send_report(surface, keyboard_report(usages), err)) {
        return false;
    }
    if (hold_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
    }
    // 位图表示当前完整按键状态；空集合将本次按住的所有键释放。
    return send_report(surface, keyboard_report({ }), err);
}

bool Service::press_chord(uint64_t surface, const std::vector<uint16_t>& usages, int hold_ms, std::string& err)
{
    std::vector<uint16_t> modifiers;
    for (const uint16_t u : usages) {
        if (u >= 0xE0 && u <= 0xE7) {
            modifiers.push_back(u);
        }
    }
    // 已测设备需要先接收修饰键状态，再接收主键，才能识别组合效果。
    if (!modifiers.empty() && !send_report(surface, keyboard_report(modifiers), err)) {
        return false;
    }
    if (!send_report(surface, keyboard_report(usages), err)) {
        return false;
    }
    if (hold_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
    }
    return send_report(surface, keyboard_report({ }), err);
}

bool Service::type_text(const std::string& text, int hold_ms, std::string& err)
{
    for (const auto& usages : text_reports(text)) {
        if (!send_report(kSurfaceKeyboard, keyboard_report(usages), err)) {
            return false;
        }
        if (hold_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
        }
    }
    return true;
}

std::unique_ptr<Buttons> Buttons::open(maa::ios::remote::Device& device, std::string& err, bool verbose)
{
    if (!device.rsd().has_service(kIndigoServiceName)) {
        err = MAA_IOS_TR("RSD directory missing ") + std::string(kIndigoServiceName);
        return nullptr;
    }
    auto conn = device.connect(kIndigoServiceName, err, verbose);
    if (conn == nullptr) {
        return nullptr;
    }
    return std::unique_ptr<Buttons>(new Buttons(std::move(conn)));
}

bool Buttons::send(uint64_t state, uint16_t usage_page, uint16_t usage_code, std::string& err)
{
    xpc::Value payload = xpc::make_dict();
    xpc::dict_set(payload, "state", xpc::make_uint64(state));
    xpc::dict_set(payload, "usagePage", xpc::make_uint64(usage_page));
    xpc::dict_set(payload, "usageCode", xpc::make_uint64(usage_code));
    return conn_->send_only(request(kButtonFeature, "IndigoButtonEvent", std::move(payload)), err);
}

bool Buttons::down(uint16_t usage_page, uint16_t usage_code, std::string& err)
{
    return send(kButtonStateDown, usage_page, usage_code, err);
}

bool Buttons::press(uint16_t usage_page, uint16_t usage_code, int hold_ms, std::string& err)
{
    // 按下和抬起各发送一条消息，不等待设备确认，间隔由调用方传入。
    // 已有设备测试中，小于 30 ms 的间隔偶有不响应；这里不设默认值或下限。
    if (!down(usage_page, usage_code, err)) {
        return false;
    }
    if (hold_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
    }
    return release(usage_page, usage_code, err);
}

bool Buttons::release(uint16_t usage_page, uint16_t usage_code, std::string& err)
{
    return send(kButtonStateUp, usage_page, usage_code, err);
}

std::vector<std::vector<uint16_t>> text_reports(const std::string& text)
{
    // usage 来自 USB-IF HID Usage Tables page 0x07，字符映射采用 US 键盘布局。
    // shifted 表示生成该字符需要先按左 Shift，再按对应的主键。
    struct Entry
    {
        char ch;
        uint16_t usage;
        bool shifted;
    };

    static const Entry kTable[] = {
        { ' ', key::kSpace, false },
        { '\t', key::kTab, false },
        { '\n', key::kEnter, false },
        // US 布局中数字键配合 Shift 生成的标点。
        { '!', key::k1, true },
        { '@', static_cast<uint16_t>(key::k1 + 1), true },
        { '#', static_cast<uint16_t>(key::k1 + 2), true },
        { '$', static_cast<uint16_t>(key::k1 + 3), true },
        { '%', static_cast<uint16_t>(key::k1 + 4), true },
        { '^', static_cast<uint16_t>(key::k1 + 5), true },
        { '&', static_cast<uint16_t>(key::k1 + 6), true },
        { '*', static_cast<uint16_t>(key::k1 + 7), true },
        { '(', static_cast<uint16_t>(key::k1 + 8), true },
        { ')', key::k0, true },
        // 其它标点按 page 0x07 的键码列出，同一键的普通字符和上档字符共用 usage。
        { '-', 0x2D, false },
        { '_', 0x2D, true },
        { '=', 0x2E, false },
        { '+', 0x2E, true },
        { '[', 0x2F, false },
        { '{', 0x2F, true },
        { ']', 0x30, false },
        { '}', 0x30, true },
        { '\\', 0x31, false },
        { '|', 0x31, true },
        { ';', 0x33, false },
        { ':', 0x33, true },
        { '\'', 0x34, false },
        { '"', 0x34, true },
        { '`', 0x35, false },
        { '~', 0x35, true },
        { ',', 0x36, false },
        { '<', 0x36, true },
        { '.', 0x37, false },
        { '>', 0x37, true },
        { '/', 0x38, false },
        { '?', 0x38, true },
    };

    std::vector<std::vector<uint16_t>> out;
    for (const char ch : text) {
        uint16_t usage = 0;
        bool shifted = false;
        if (ch >= 'a' && ch <= 'z') {
            usage = static_cast<uint16_t>(key::kA + (ch - 'a'));
        }
        else if (ch >= 'A' && ch <= 'Z') {
            usage = static_cast<uint16_t>(key::kA + (ch - 'A'));
            shifted = true;
        }
        else if (ch >= '1' && ch <= '9') {
            usage = static_cast<uint16_t>(key::k1 + (ch - '1'));
        }
        else if (ch == '0') {
            usage = key::k0;
        }
        else {
            for (const auto& e : kTable) {
                if (e.ch == ch) {
                    usage = e.usage;
                    shifted = e.shifted;
                    break;
                }
            }
        }
        if (usage == 0) {
            continue; // 按字节跳过未支持字符，不产生报告或错误。
        }
        if (shifted) {
            // 分两条报告建立 Shift 和主键状态；已有设备同时接收二者时，
            // 上档效果未生效。该顺序与 press_chord() 的修饰键处理一致。
            out.push_back({ key::kShiftLeft });
            out.push_back({ key::kShiftLeft, usage });
        }
        else {
            out.push_back({ usage });
        }
        out.push_back({ }); // 每个字符结束时释放全部按键。
    }
    return out;
}

} // namespace maa::ios::hid
