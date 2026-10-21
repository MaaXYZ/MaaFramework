#pragma once

#include <cstring>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <meojson/json.hpp>

#include <MaaFramework/MaaAPI.h>
#ifdef MAA_JS_WITH_TOOLKIT
#include <MaaToolkit/MaaToolkitAPI.h>
#endif

#include "../foundation/spec.h"
#include "controller.h"

struct IOSPairingView
{
    std::string udid;
    std::string host_identifier;
    std::string peer_identifier;
    MaaIOSWiFiPairing value { };

    IOSPairingView(const IOSPairingView&) = delete;
    IOSPairingView& operator=(const IOSPairingView&) = delete;

    explicit IOSPairingView(maajs::ObjectType object)
    {
        udid = maajs::JSConvert<std::string>::from_value(object["udid"].AsValue());
        host_identifier = maajs::JSConvert<std::string>::from_value(object["host_identifier"].AsValue());
        peer_identifier = maajs::JSConvert<std::string>::from_value(object["peer_identifier"].AsValue());
        if (udid.find('\0') != std::string::npos || host_identifier.find('\0') != std::string::npos
            || peer_identifier.find('\0') != std::string::npos) {
            throw maajs::MaaError { "iOS pairing identities must not contain NUL" };
        }
        value.udid = udid.c_str();
        value.host_identifier = host_identifier.c_str();
        if (peer_identifier.empty() || peer_identifier.size() > 4096) {
            throw maajs::MaaError { "Invalid iOS pairing peer identifier length" };
        }
        value.peer_identifier = peer_identifier.c_str();
        std::memcpy(value.host_private_key, bytes(object, "host_private_key", 32), 32);
        std::memcpy(value.host_public_key, bytes(object, "host_public_key", 32), 32);
        std::memcpy(value.peer_public_key, bytes(object, "peer_public_key", 32), 32);
        const maajs::ValueType irk = object["peer_alt_irk"].AsValue();
        if (!irk.IsUndefined() && !irk.IsNull()) {
            std::memcpy(value.peer_alt_irk, bytes(object, "peer_alt_irk", 16), 16);
            value.has_peer_alt_irk = true;
        }
    }

    static const uint8_t* bytes(maajs::ObjectType object, const char* name, size_t size)
    {
        auto buffer = maajs::JSConvert<maajs::ArrayBufferType>::from_value(object[name].AsValue());
        if (buffer.ByteLength() != size) {
            throw maajs::MaaError { std::format("{} must contain {} bytes", name, size) };
        }
        return static_cast<const uint8_t*>(buffer.Data());
    }
};

#ifdef MAA_JS_WITH_TOOLKIT
inline std::optional<std::vector<IOSDevice>>
    find_ios_devices(int64_t timeout, MaaIOSConnectionMethod method, const std::vector<std::shared_ptr<IOSPairingView>>& pairings)
{
    auto list = MaaToolkitIOSDeviceListCreate();
    if (!list) {
        return std::nullopt;
    }
    std::unique_ptr<MaaToolkitIOSDeviceList, decltype(&MaaToolkitIOSDeviceListDestroy)> owned(list, MaaToolkitIOSDeviceListDestroy);
    const auto options = json::value(json::object { { "timeout_ms", timeout } }).to_string();
    std::vector<const MaaIOSWiFiPairing*> pointers;
    for (const auto& pairing : pairings) {
        pointers.push_back(&pairing->value);
    }
    if (!MaaToolkitIOSDeviceFindWithOptions(method, pointers.data(), pointers.size(), options.c_str(), list)) {
        return std::nullopt;
    }
    std::vector<IOSDevice> result;
    for (MaaSize i = 0; i < MaaToolkitIOSDeviceListSize(list); ++i) {
        auto device = MaaToolkitIOSDeviceListAt(list, i);
        result.emplace_back(
            MaaToolkitIOSDeviceGetName(device),
            MaaToolkitIOSDeviceGetUdid(device),
            MaaToolkitIOSDeviceGetScreencapMethods(device),
            MaaToolkitIOSDeviceGetConnections(device));
    }
    return result;
}

inline maajs::ValueType parse_ios_pairing_record(maajs::EnvType env, const std::string& record)
{
    if (record.find('\0') != std::string::npos) {
        throw maajs::MaaError { "iOS pairing record must not contain NUL" };
    }
    std::unique_ptr<MaaIOSWiFiPairing, decltype(&MaaToolkitIOSDeviceDestroyPairing)> pairing(
        MaaToolkitIOSDeviceParsePairingRecord(record.c_str()),
        MaaToolkitIOSDeviceDestroyPairing);
    if (!pairing) {
        throw maajs::MaaError { "Invalid iOS pairing record or iOS support is disabled" };
    }
    auto result = maajs::ObjectType::New(env);
    result["udid"] = maajs::JSConvert<std::string>::to_value(env, pairing->udid);
    result["host_identifier"] = maajs::JSConvert<std::string>::to_value(env, pairing->host_identifier);
    result["peer_identifier"] = maajs::JSConvert<std::string>::to_value(env, pairing->peer_identifier);
    const auto put_bytes = [&](const char* property_name, const uint8_t* data, size_t size) {
        auto buffer = maajs::ArrayBufferType::New(env, size);
        std::memcpy(buffer.Data(), data, size);
        result[property_name] = buffer;
    };
    put_bytes("host_private_key", pairing->host_private_key, 32);
    put_bytes("host_public_key", pairing->host_public_key, 32);
    put_bytes("peer_public_key", pairing->peer_public_key, 32);
    if (pairing->has_peer_alt_irk) {
        put_bytes("peer_alt_irk", pairing->peer_alt_irk, 16);
    }
    return result;
}

inline std::string format_ios_pairing_record(maajs::ObjectType value)
{
    IOSPairingView pairing(value);
    auto* record = MaaStringBufferCreate();
    if (!record) {
        throw maajs::MaaError { "Cannot allocate iOS pairing record buffer" };
    }
    std::unique_ptr<MaaStringBuffer, decltype(&MaaStringBufferDestroy)> owned(record, MaaStringBufferDestroy);
    if (!MaaToolkitIOSDeviceFormatPairingRecord(&pairing.value, record)) {
        throw maajs::MaaError { "Invalid iOS pairing credentials or iOS support is disabled" };
    }
    return MaaStringBufferGet(record);
}
#endif
