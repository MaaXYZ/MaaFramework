#include "remote/PairingChannel.h"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <limits>
#include <utility>

#include "wifi/Crypto.h"

namespace maa::ios::remote
{

namespace
{

/// 这个字符串字段是不是"其实是二进制"的那两处之一。
///
/// 判据用键名与它的上一级/上上级键名，不用完整路径：信封的形状是固定的
/// （`message.streamEncrypted._0` 与 `message.plain._0.event._0.pairingData._0.data`），
/// 而深层路径比对要把整条路径带进递归，读起来更绕却不多一分准确。
bool is_binary_field(std::string_view key, std::string_view parent, std::string_view grandparent)
{
    if (key == "_0" && parent == "streamEncrypted") {
        return true;
    }
    return key == "data" && grandparent == "pairingData";
}

bool to_xpc(
    const json::Value& value,
    std::string_view key,
    std::string_view parent,
    std::string_view grandparent,
    xpc::Value& out,
    std::string& err);

bool to_xpc_children(const json::Value& value, std::string_view key, std::string_view parent, xpc::Value& out, std::string& err)
{
    for (const auto& item : value.as_array()) {
        xpc::Value child;
        // 数组元素没有键名，所以它的"上一级"就是这个数组自己的键。
        if (!to_xpc(item, "", key, parent, child, err)) {
            return false;
        }
        xpc::array_push(out, std::move(child));
    }
    return true;
}

bool to_xpc(
    const json::Value& value,
    std::string_view key,
    std::string_view parent,
    std::string_view grandparent,
    xpc::Value& out,
    std::string& err)
{
    switch (value.type()) {
    case json::Value::value_type::null:
        out = xpc::make_null();
        return true;
    case json::Value::value_type::boolean:
        out = xpc::make_bool(value.as_boolean());
        return true;
    case json::Value::value_type::number:
        if (value.is<int64_t>()) {
            const auto number = value.as<int64_t>();
            out = key == "sequenceNumber" ? xpc::make_uint64(static_cast<uint64_t>(number)) : xpc::make_int64(number);
            return true;
        }
        if (value.is<uint64_t>()) {
            if (key != "sequenceNumber") {
                err = "JSON integer exceeds XPC int64 range";
                return false;
            }
            out = xpc::make_uint64(value.as<uint64_t>());
            return true;
        }
        if (const auto number = json::as_double(value)) {
            out = xpc::make_double(*number);
            return true;
        }
        err = "JSON number exceeds XPC range";
        return false;
    case json::Value::value_type::string: {
        if (!is_binary_field(key, parent, grandparent)) {
            out = xpc::make_string(value.as_string());
            return true;
        }
        const auto bytes = wifi::b64_decode(value.as_string(), err);
        if (!bytes) {
            err = "Field " + std::string(key) + " must contain base64 binary data; decode failed: " + err;
            return false;
        }
        out = xpc::make_data(std::move(*bytes));
        return true;
    }
    case json::Value::value_type::array:
        out = xpc::make_array();
        return to_xpc_children(value, key, parent, out, err);
    case json::Value::value_type::object:
        out = xpc::make_dict();
        for (const auto& kv : value.as_object()) {
            xpc::Value child;
            if (!to_xpc(kv.second, kv.first, key, parent, child, err)) {
                return false;
            }
            xpc::dict_set(out, kv.first, std::move(child));
        }
        return true;
    default:
        break;
    }
    err = "Unknown JSON value type";
    return false;
}

bool from_xpc(const xpc::Value& value, json::Value& out, std::string& err);

std::string uuid_text(const wifi::Bytes& bytes)
{
    static constexpr int kGroup[] = { 4, 2, 2, 2, 6 };
    std::string text;
    size_t at = 0;
    for (size_t g = 0; g < sizeof(kGroup) / sizeof(kGroup[0]); ++g) {
        if (g != 0) {
            text += '-';
        }
        for (int i = 0; i < kGroup[g]; ++i, ++at) {
            char buf[3] = { 0 };
            std::snprintf(buf, sizeof(buf), "%02x", bytes[at]);
            text += buf;
        }
    }
    return text;
}

bool from_xpc(const xpc::Value& value, json::Value& out, std::string& err)
{
    switch (value.type) {
    case xpc::Type::Null:
        out = json::Value { };
        return true;
    case xpc::Type::Bool:
        out = wifi::j_bool(value.boolean);
        return true;
    case xpc::Type::Int64:
        out = wifi::j_int(value.int64);
        return true;
    case xpc::Type::UInt64:
        out = value.uint64;
        return true;
    case xpc::Type::Double: {
        if (!std::isfinite(value.real)) {
            err = "XPC double is not finite";
            return false;
        }
        // Preserve precision and the floating-point wire type, including integral doubles.
        char buffer[64];
        const auto [end, ec] = std::to_chars(
            buffer,
            buffer + sizeof(buffer),
            value.real,
            std::chars_format::general,
            std::numeric_limits<double>::max_digits10);
        if (ec != std::errc { }) {
            err = "Cannot represent XPC double as JSON";
            return false;
        }
        std::string number(buffer, end);
        if (number.find_first_of(".eE") == std::string::npos) {
            number += ".0";
        }
        out = json::Value(json::Value::value_type::number, std::move(number));
        return true;
    }
    case xpc::Type::Date:
        out = wifi::j_int(static_cast<int64_t>(value.uint64));
        return true;
    case xpc::Type::Data:
        // 字节流载体上这个位置放的是 base64 文本，两种载体要交出同一种形状，
        // 配对逻辑才不用管自己在哪一种上。
        out = wifi::j_str(wifi::b64_encode(value.data));
        return true;
    case xpc::Type::String:
        out = wifi::j_str(value.string);
        return true;
    case xpc::Type::Uuid:
        // 设备的 peerDeviceInfo.identifier 在字节流载体上是字符串，在这边可能是
        // UUID 对象。还原成同样的 8-4-4-4-12 文本，上层读到的才是同一个东西。
        if (value.data.size() != 16) {
            err = "UUID field must contain 16 bytes";
            return false;
        }
        out = wifi::j_str(uuid_text(value.data));
        return true;
    case xpc::Type::Array:
        out = wifi::j_arr({ });
        for (const auto& item : value.array) {
            json::Value child;
            if (!from_xpc(item, child, err)) {
                return false;
            }
            out.emplace(std::move(child));
        }
        return true;
    case xpc::Type::Dict:
        out = wifi::j_obj({ });
        for (const auto& entry : value.dict) {
            json::Value child;
            if (!from_xpc(entry.value, child, err)) {
                return false;
            }
            out.emplace(entry.key, std::move(child));
        }
        return true;
    case xpc::Type::FileTransfer:
        err = "File transfer placeholder is not supported on the pairing channel";
        return false;
    }
    err = "Unknown XPC type marker";
    return false;
}

} // namespace

std::optional<xpc::Value> json_to_xpc(const json::Value& value, std::string& err)
{
    xpc::Value out;
    if (!to_xpc(value, "", "", "", out, err)) {
        return std::nullopt;
    }
    return out;
}

std::optional<json::Value> xpc_to_json(const xpc::Value& value, std::string& err)
{
    json::Value out;
    if (!from_xpc(value, out, err)) {
        return std::nullopt;
    }
    return out;
}

bool XpcPairingCarrier::write_envelope(const json::Value& envelope, std::string& err)
{
    const auto value = json_to_xpc(envelope, err);
    if (!value) {
        return false;
    }
    xpc::Value body = xpc::make_dict();
    xpc::dict_set(body, "mangledTypeName", xpc::make_string(std::string(kPairingEnvelopeType)));
    xpc::dict_set(body, "value", std::move(*value));
    return conn_.send_only(body, err);
}

bool XpcPairingCarrier::wait_one(int ms, xpc::Value& out, std::string& err)
{
    switch (conn_.wait_message(out, ms, err)) {
    case Channel::Wait::Message:
        return true;
    case Channel::Wait::Timeout:
        if (err.empty()) {
            err = "Pairing reply timed out (" + std::to_string(ms) + " ms without device data)";
        }
        return false;
    case Channel::Wait::Broken:
        if (err.empty()) {
            err = "Pairing control channel disconnected";
        }
        return false;
    }
    err = "Pairing control channel disconnected";
    return false;
}

std::optional<json::Value> XpcPairingCarrier::read_envelope(std::string& err)
{
    xpc::Value body;
    if (pending_.has_value()) {
        body = std::move(*pending_);
        pending_.reset();
    }
    else if (!wait_one(timeout_ms_, body, err)) {
        return std::nullopt;
    }
    const xpc::Value* value = body.find("value");
    if (value == nullptr) {
        err = "Channel received unexpected type; expected " + std::string(kPairingEnvelopeType)
              + " (value field missing): " + xpc::describe(body).substr(0, 400);
        return std::nullopt;
    }
    return xpc_to_json(*value, err);
}

bool XpcPairingCarrier::wait_readable(int ms, std::string& err)
{
    if (pending_.has_value()) {
        return true;
    }
    xpc::Value body;
    if (!wait_one(ms, body, err)) {
        return false;
    }
    pending_ = std::move(body);
    return true;
}

} // namespace maa::ios::remote
