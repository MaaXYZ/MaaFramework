#include "remote/Pasteboard.h"
#include "i18n/Translation.h"

#include <cstdio>
#include <vector>

#include "remote/Device.h"

namespace maa::ios::remote
{
namespace
{

constexpr std::string_view kServiceName = "com.apple.coredevice.pasteboardservice";
constexpr std::string_view kUti = "public.utf8-plain-text";

bool cancelled(std::stop_token cancel, std::string& err)
{
    if (!cancel.stop_requested()) {
        return false;
    }
    err = MAA_IOS_TR("Clipboard request cancelled");
    return true;
}

bool valid_request(std::stop_token cancel, int reply_timeout_ms, std::string& err)
{
    if (cancelled(cancel, err)) {
        return false;
    }
    if (reply_timeout_ms < 0) {
        err = MAA_IOS_TR("Clipboard reply timeout must not be negative");
        return false;
    }
    return true;
}

} // namespace

xpc::Value Pasteboard::build_pull()
{
    auto msg = xpc::make_dict();
    xpc::dict_set(msg, "command", xpc::make_string("PULL"));
    xpc::dict_set(msg, "pasteboardName", xpc::make_string("general"));
    // dataPolicy 使用单键枚举表示；已测服务拒绝空字典和多键字典。
    // allResolved 请求将可解析的表示形式以内联 Data 返回。
    auto all_resolved = xpc::make_dict();
    auto data_policy = xpc::make_dict();
    xpc::dict_set(data_policy, "allResolved", std::move(all_resolved));
    xpc::dict_set(msg, "dataPolicy", std::move(data_policy));
    return msg;
}

xpc::Value Pasteboard::build_set(const std::string& text)
{
    auto representation = xpc::make_dict();
    xpc::dict_set(representation, "data", xpc::make_data(std::vector<uint8_t>(text.begin(), text.end())));

    auto data = xpc::make_dict();
    xpc::dict_set(data, std::string(kUti), std::move(representation));

    auto types = xpc::make_array();
    xpc::array_push(types, xpc::make_string(std::string(kUti)));

    auto item = xpc::make_dict();
    xpc::dict_set(item, "types", std::move(types));
    xpc::dict_set(item, "data", std::move(data));

    auto items = xpc::make_array();
    xpc::array_push(items, std::move(item));

    auto msg = xpc::make_dict();
    xpc::dict_set(msg, "command", xpc::make_string("SET"));
    xpc::dict_set(msg, "pasteboardName", xpc::make_string("general"));
    xpc::dict_set(msg, "items", std::move(items));
    return msg;
}

std::optional<std::string> Pasteboard::find_text(const xpc::Value& reply)
{
    const auto* snapshot = reply.find("pasteboard");
    if (snapshot == nullptr) {
        return std::nullopt;
    }
    const auto* items = snapshot->find("items");
    if (items == nullptr) {
        return std::nullopt;
    }
    for (const auto& item : items->array) {
        const auto* data = item.find("data");
        if (data == nullptr) {
            continue;
        }
        const auto* rep = data->find(kUti);
        if (rep == nullptr) {
            continue;
        }
        const auto* bytes = rep->find("data");
        if (bytes == nullptr || bytes->type != xpc::Type::Data) {
            continue;
        }
        return std::string(bytes->data.begin(), bytes->data.end());
    }
    return std::nullopt;
}

bool Pasteboard::set_text(
    Device& device,
    const std::string& text,
    std::string& err,
    bool verbose,
    std::stop_token cancel,
    int reply_timeout_ms)
{
    if (!valid_request(cancel, reply_timeout_ms, err)) {
        return false;
    }
    auto conn = device.connect(kServiceName, err, verbose, cancel);
    if (cancelled(cancel, err)) {
        return false;
    }
    if (conn == nullptr) {
        return false;
    }
    xpc::Value reply;
    if (!conn->call(build_set(text), reply, reply_timeout_ms, err)) {
        if (cancelled(cancel, err)) {
            return false;
        }
        err = MAA_IOS_TR("SET failed: ") + err;
        return false;
    }
    if (cancelled(cancel, err)) {
        return false;
    }
    // 检查回复命令。SET_REPLY 本身不证明设备已保存文本，落地验证需另行读回。
    if (reply.at("command").as_string_or("") != "SET_REPLY") {
        err = MAA_IOS_TR("SET response is not SET_REPLY: ") + xpc::describe(reply).substr(0, 300);
        return false;
    }
    return !cancelled(cancel, err);
}

bool Pasteboard::get_text(Device& device, std::string& out, std::string& err, bool verbose, std::stop_token cancel, int reply_timeout_ms)
{
    if (!valid_request(cancel, reply_timeout_ms, err)) {
        return false;
    }
    auto conn = device.connect(kServiceName, err, verbose, cancel);
    if (cancelled(cancel, err)) {
        return false;
    }
    if (conn == nullptr) {
        return false;
    }
    xpc::Value reply;
    if (!conn->call(build_pull(), reply, reply_timeout_ms, err)) {
        if (cancelled(cancel, err)) {
            return false;
        }
        err = MAA_IOS_TR("PULL failed: ") + err;
        return false;
    }
    if (cancelled(cancel, err)) {
        return false;
    }
    if (reply.at("command").as_string_or("") != "PULL_REPLY") {
        err = MAA_IOS_TR("PULL response is not PULL_REPLY: ") + xpc::describe(reply).substr(0, 300);
        return false;
    }
    const auto text = find_text(reply);
    if (!text) {
        // 缺少 UTF-8 Data 表示时保留有限长度的回复描述，帮助区分只有其他类型内容
        // 和设备返回错误；存在但零长度的 Data 已由 find_text 作为空文本接受。
        err = MAA_IOS_TR("Response has no plain-text representation: ") + xpc::describe(reply).substr(0, 400);
        return false;
    }
    if (cancelled(cancel, err)) {
        return false;
    }
    out = *text;
    return true;
}

} // namespace maa::ios::remote
