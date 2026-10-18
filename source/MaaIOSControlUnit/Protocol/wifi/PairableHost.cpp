#include "wifi/PairableHost.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <map>
#include <set>
#include <utility>

#include "wifi/Opack.h"
#include "wifi/Srp.h"
#include "wifi/Tlv.h"

namespace maa::ios::wifi
{
namespace
{

using Fields = std::map<uint8_t, Bytes>;

bool label_valid(std::string_view value, bool allow_empty = false)
{
    return (allow_empty || !value.empty()) && value.size() <= 255
           && std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 0x20 || c == 0x7f; });
}

std::optional<int64_t> integer(const json::Value* value)
{
    if (value == nullptr || !value->is<int64_t>()) {
        return std::nullopt;
    }
    return value->as<int64_t>();
}

const json::Value* path(const json::Value& root, std::initializer_list<std::string_view> keys)
{
    const json::Value* value = &root;
    for (const auto key : keys) {
        value = json::find(*value, key);
        if (value == nullptr) {
            return nullptr;
        }
    }
    return value;
}

class Conversation
{
public:
    explicit Conversation(EnvelopeCarrier& carrier)
        : carrier_(carrier)
    {
    }

    std::optional<json::Value> receive(std::string& err)
    {
        const auto envelope = carrier_.read_envelope(err);
        if (!envelope) {
            return std::nullopt;
        }
        const auto sequence = integer(json::find(*envelope, "sequenceNumber"));
        const auto* origin = json::find(*envelope, "originatedBy");
        const auto* message = json::find(*envelope, "message");
        const auto* plain = path(*envelope, { "message", "plain" });
        const auto* body = path(*envelope, { "message", "plain", "_0" });
        if (!envelope->is_object() || envelope->as_object().size() != 3 || !sequence || *sequence != incoming_ || origin == nullptr
            || !origin->is_string() || *origin != "host" || message == nullptr || !message->is_object() || message->as_object().size() != 1
            || plain == nullptr || !plain->is_object() || plain->as_object().size() != 1 || body == nullptr || !body->is_object()) {
            err = "Pairable host expected a plain host envelope with the next sequence";
            return std::nullopt;
        }
        ++incoming_;
        return std::optional<json::Value>(std::in_place, *body);
    }

    bool send(const json::Value& body, std::string& err)
    {
        const json::Value envelope = { { "message", { { "plain", { { "_0", body } } } } },
                                       { "originatedBy", "device" },
                                       { "sequenceNumber", outgoing_ } };
        if (!carrier_.write_envelope(envelope, err)) {
            return false;
        }
        ++outgoing_;
        return true;
    }

    std::optional<Fields> pairing(uint8_t state, bool new_session, std::string& err)
    {
        const auto body = receive(err);
        if (!body) {
            return std::nullopt;
        }
        const auto* event = path(*body, { "event", "_0" });
        const auto* pairing = path(*body, { "event", "_0", "pairingData" });
        const auto* data = path(*body, { "event", "_0", "pairingData", "_0" });
        const auto* kind = data != nullptr ? json::find(*data, "kind") : nullptr;
        const auto* start = data != nullptr ? json::find(*data, "startNewSession") : nullptr;
        const auto* encoded = data != nullptr ? json::find(*data, "data") : nullptr;
        if (body->as_object().size() != 1 || event == nullptr || !event->is_object() || event->as_object().size() != 1 || pairing == nullptr
            || !pairing->is_object() || pairing->as_object().size() != 1 || data == nullptr || !data->is_object() || kind == nullptr
            || !kind->is_string() || *kind != "setupManualPairing" || start == nullptr || !start->is_boolean()
            || start->as_boolean() != new_session || encoded == nullptr || !encoded->is_string()
            || encoded->as_string_view().size() > 32768) {
            err = "Pairable host expected setupManualPairing data with the correct session flag";
            return std::nullopt;
        }
        const auto bytes = b64_decode(encoded->as_string_view(), err);
        if (!bytes) {
            return std::nullopt;
        }
        auto fields = strict_tlv(*bytes, err);
        if (!fields) {
            return std::nullopt;
        }
        const auto* phase = tlv_get(*fields, TlvType::State);
        if (phase == nullptr || *phase != Bytes { state } || tlv_get(*fields, TlvType::Error) != nullptr) {
            err = "Pairable host received an invalid pairing state or peer error";
            return std::nullopt;
        }
        return fields;
    }

    bool send_pairing(const Bytes& tlv, std::string& err)
    {
        const json::Value body = {
            { "event",
              { { "_0",
                  { { "pairingData",
                      { { "_0",
                          { { "data", b64_encode(tlv) }, { "startNewSession", false }, { "kind", "setupManualPairing" } } } } } } } } }
        };
        return send(body, err);
    }

    static std::optional<Fields> strict_tlv(const Bytes& bytes, std::string& err)
    {
        err.clear();
        const auto fields = tlv_parse(bytes, err);
        if (!err.empty()) {
            return std::nullopt;
        }
        std::set<uint8_t> seen;
        uint8_t previous_type = 0;
        size_t previous_length = 0;
        for (size_t offset = 0; offset < bytes.size();) {
            const uint8_t type = bytes[offset];
            const size_t length = bytes[offset + 1];
            if (seen.contains(type) && (previous_type != type || previous_length != 255 || length == 0)) {
                err = "Pairable host TLV repeated without a valid continuation";
                return std::nullopt;
            }
            seen.insert(type);
            previous_type = type;
            previous_length = length;
            offset += length + 2;
        }
        return fields;
    }

private:
    EnvelopeCarrier& carrier_;
    int64_t incoming_ = 0;
    int64_t outgoing_ = 0;
};

std::optional<PairableHostResult> device_identity(const Bytes& key, const Bytes& plain, std::string& err)
{
    const auto fields = Conversation::strict_tlv(plain, err);
    if (!fields) {
        return std::nullopt;
    }
    const auto* identifier = tlv_get(*fields, TlvType::Identifier);
    const auto* public_key = tlv_get(*fields, TlvType::PublicKey);
    const auto* signature = tlv_get(*fields, TlvType::Signature);
    const auto* info = tlv_get(*fields, TlvType::Info);
    if (identifier == nullptr || identifier->empty() || identifier->size() > 1024 || public_key == nullptr || public_key->size() != 32
        || signature == nullptr || signature->size() != 64 || info == nullptr || info->empty() || info->size() > 16384) {
        err = "Pairable host M5 is missing a complete signed device identity";
        return std::nullopt;
    }
    const auto prefix = hkdf_sha512(key, "Pair-Setup-Controller-Sign-Salt", "Pair-Setup-Controller-Sign-Info", 32, err);
    if (!prefix) {
        return std::nullopt;
    }
    Bytes signed_data = *prefix;
    signed_data.insert(signed_data.end(), identifier->begin(), identifier->end());
    signed_data.insert(signed_data.end(), public_key->begin(), public_key->end());
    if (!ed25519_verify(sv(*public_key), signed_data, *signature, err)) {
        err = "Pairable host M5 device signature verification failed";
        return std::nullopt;
    }
    OpackValue decoded;
    if (!opack_decode(*info, decoded, err)) {
        return std::nullopt;
    }
    std::set<std::string> keys;
    if (decoded.kind != OpackValue::Kind::kDict) {
        err = "Pairable host M5 INFO is not a dictionary";
        return std::nullopt;
    }
    for (const auto& [name, value] : decoded.dict) {
        (void)value;
        if (name.kind != OpackValue::Kind::kString || !keys.insert(name.str).second) {
            err = "Pairable host M5 INFO has invalid or duplicate keys";
            return std::nullopt;
        }
    }
    const auto* udid = decoded.find("remotepairing_udid");
    const auto* irk = decoded.find("altIRK");
    const auto* account = decoded.find("accountID");
    if (udid == nullptr || udid->kind != OpackValue::Kind::kString || !label_valid(udid->str) || irk == nullptr
        || irk->kind != OpackValue::Kind::kBytes || irk->bytes.size() != 16
        || (account != nullptr && (account->kind != OpackValue::Kind::kString || bytes_of(account->str) != *identifier))) {
        err = "Pairable host M5 INFO lacks a valid UDID/altIRK or contradicts the signed Identifier";
        return std::nullopt;
    }
    PairableHostResult result;
    result.record.udid = udid->str;
    result.record.peer_identifier.assign(identifier->begin(), identifier->end());
    result.record.peer_public_key = *public_key;
    result.record.peer_alt_irk = irk->bytes;
    for (const auto& [name, output] : std::array { std::pair { "name", &result.peer_name }, std::pair { "model", &result.peer_model } }) {
        if (const auto* value = decoded.find(name)) {
            if (value->kind != OpackValue::Kind::kString || !label_valid(value->str)) {
                err = "Pairable host M5 INFO has invalid device labels";
                return std::nullopt;
            }
            *output = value->str;
        }
    }
    return result;
}

} // namespace

std::optional<PairableHostResult> accept_pairable_host(
    EnvelopeCarrier& carrier,
    const PairableHostOptions& options,
    const PairablePinCallback& display_pin,
    std::string& err,
    const PairableAdmissionCallback& admit)
{
    err.clear();
    if (!label_valid(options.host_identifier) || !label_valid(options.host_name) || !label_valid(options.host_model)
        || !label_valid(options.host_udid, true) || options.host_alt_irk.size() != 16 || options.host_private_key.size() != 32
        || options.host_public_key.size() != 32 || options.setup_pin.size() != 6
        || !std::all_of(options.setup_pin.begin(), options.setup_pin.end(), [](char c) { return c >= '0' && c <= '9'; }) || !display_pin
        || options.minimum_wire_protocol_version < 8 || options.wire_protocol_version < options.minimum_wire_protocol_version) {
        err = "Pairable host configuration is incomplete or invalid";
        return std::nullopt;
    }
    const auto self_signature = ed25519_sign(sv(options.host_private_key), Bytes { }, err);
    if (!self_signature || !ed25519_verify(sv(options.host_public_key), Bytes { }, *self_signature, err)) {
        err = "Pairable host public key does not match its private key";
        return std::nullopt;
    }
    const auto metadata = host_metadata(options.host_identifier, err);
    if (!metadata) {
        return std::nullopt;
    }
    Conversation conversation(carrier);
    const auto request = conversation.receive(err);
    if (!request) {
        return std::nullopt;
    }
    const auto* request_branch = path(*request, { "request", "_0" });
    const auto* handshake_branch = path(*request, { "request", "_0", "handshake" });
    const auto* handshake = path(*request, { "request", "_0", "handshake", "_0" });
    const auto version = handshake != nullptr ? integer(json::find(*handshake, "wireProtocolVersion")) : std::nullopt;
    const auto* host_options = handshake != nullptr ? json::find(*handshake, "hostOptions") : nullptr;
    const auto* verify = host_options != nullptr ? json::find(*host_options, "attemptPairVerify") : nullptr;
    const auto* minimum = handshake != nullptr ? json::find(*handshake, "minimumSupportedWireProtocolVersion") : nullptr;
    const auto peer_minimum = minimum != nullptr ? integer(minimum) : std::optional<int64_t> { 8 };
    if (request->as_object().size() != 1 || request_branch == nullptr || !request_branch->is_object()
        || request_branch->as_object().size() != 1 || handshake_branch == nullptr || !handshake_branch->is_object()
        || handshake_branch->as_object().size() != 1 || handshake == nullptr || !handshake->is_object() || !version
        || *version < options.minimum_wire_protocol_version || !peer_minimum || *peer_minimum < 8 || *peer_minimum > *version
        || *peer_minimum > options.wire_protocol_version || verify == nullptr || !verify->is_boolean() || verify->as_boolean()) {
        err = "Pairable host rejected an incompatible or pair-verify handshake";
        return std::nullopt;
    }
    const json::Value response = { { "response",
                                     { { "forRequestIdentifier", 0 },
                                       { "_1",
                                         { { "handshake",
                                             { { "_0",
                                                 { { "wireProtocolVersion", options.wire_protocol_version },
                                                   { "minimumSupportedWireProtocolVersion", options.minimum_wire_protocol_version },
                                                   { "deviceOptions",
                                                     { { "allowsPairSetup", true },
                                                       { "allowsPinlessPairing", false },
                                                       { "allowsIncomingTunnelConnections", false },
                                                       { "allowsUpgradeOfLockdownPairings", false },
                                                       { "allowsSharingSensitiveInfo", false } } },
                                                   { "peerDeviceInfo",
                                                     { { "udid", options.host_udid },
                                                       { "deviceKVSIncludesSensitiveInfo", false },
                                                       { "identifier", options.host_identifier },
                                                       { "name", options.host_name },
                                                       { "model", options.host_model } } } } } } } } } } } };
    if (!conversation.send(response, err)) {
        return std::nullopt;
    }
    const auto m1 = conversation.pairing(1, true, err);
    if (!m1) {
        return std::nullopt;
    }
    const auto* method = tlv_get(*m1, TlvType::Method);
    if (method == nullptr || *method != Bytes { 0 }) {
        err = "Pairable host M1 has an unsupported setup method";
        return std::nullopt;
    }
    SrpServer server("Pair-Setup", options.setup_pin);
    auto salt = random_bytes(16, err);
    if (!salt) {
        return std::nullopt;
    }
    (*salt)[0] |= 0x80;
    // Keep the challenge's integer encodings at their protocol widths. This
    // resamples local randomness only, before any M2 or PIN is sent/displayed.
    bool challenge_ready = false;
    for (size_t attempt = 0; attempt < 16; ++attempt) {
        if (!server.initialize(*salt, err)) {
            return std::nullopt;
        }
        if (server.server_public().size() == 384) {
            challenge_ready = true;
            break;
        }
    }
    if (!challenge_ready) {
        err = "Pairable host could not generate a full-width SRP challenge";
        return std::nullopt;
    }
    if (!display_pin(options.setup_pin, err)) {
        if (err.empty()) {
            err = "Pairable host PIN display was cancelled";
        }
        return std::nullopt;
    }
    if (!conversation.send_pairing(
            tlv_build({ { TlvType::State, Bytes { 2 } }, { TlvType::Salt, *salt }, { TlvType::PublicKey, server.server_public() } }),
            err)) {
        return std::nullopt;
    }
    const auto m3 = conversation.pairing(3, false, err);
    if (!m3) {
        return std::nullopt;
    }
    const auto* client_public = tlv_get(*m3, TlvType::PublicKey);
    const auto* client_proof = tlv_get(*m3, TlvType::Proof);
    if (client_public == nullptr || client_proof == nullptr || !server.process(*client_public, *client_proof, err)) {
        if (err.empty()) {
            err = "Pairable host M3 is missing the SRP public key or proof";
        }
        const std::string first_error = err;
        std::string ignored;
        (void)conversation.send_pairing(tlv_build({ { TlvType::State, Bytes { 4 } }, { TlvType::Error, Bytes { 2 } } }), ignored);
        err = first_error;
        return std::nullopt;
    }
    if (!conversation.send_pairing(tlv_build({ { TlvType::State, Bytes { 4 } }, { TlvType::Proof, server.server_proof() } }), err)) {
        return std::nullopt;
    }
    const auto setup_key = hkdf_sha512(server.session_key(), "Pair-Setup-Encrypt-Salt", "Pair-Setup-Encrypt-Info", 32, err);
    if (!setup_key) {
        return std::nullopt;
    }
    const auto m5 = conversation.pairing(5, false, err);
    if (!m5) {
        return std::nullopt;
    }
    const auto* sealed = tlv_get(*m5, TlvType::EncryptedData);
    if (sealed == nullptr) {
        err = "Pairable host M5 is missing encrypted identity";
        return std::nullopt;
    }
    constexpr char nonce5[] = "\x00\x00\x00\x00PS-Msg05";
    const auto plain = chacha_open(sv(*setup_key), std::string_view(nonce5, sizeof(nonce5) - 1), *sealed, err);
    if (!plain) {
        return std::nullopt;
    }
    auto result = device_identity(server.session_key(), *plain, err);
    if (!result) {
        return std::nullopt;
    }
    // Persistence checks after this function returns are too late to prevent a
    // duplicate or unexpected peer from receiving this host's M6 trust identity.
    // Unauthenticated M5 data must never reach a filesystem admission callback.
    if (admit && !admit(*result, err)) {
        if (err.empty()) {
            err = "Wi-Fi pairing returned an incomplete or unexpected device identity";
        }
        return std::nullopt;
    }
    const auto prefix = hkdf_sha512(server.session_key(), "Pair-Setup-Accessory-Sign-Salt", "Pair-Setup-Accessory-Sign-Info", 32, err);
    if (!prefix) {
        return std::nullopt;
    }
    Bytes signed_data = *prefix;
    const auto host_id = bytes_of(options.host_identifier);
    signed_data.insert(signed_data.end(), host_id.begin(), host_id.end());
    signed_data.insert(signed_data.end(), options.host_public_key.begin(), options.host_public_key.end());
    const auto signature = ed25519_sign(sv(options.host_private_key), signed_data, err);
    if (!signature) {
        return std::nullopt;
    }
    const auto mac = random_bytes(6, err);
    if (!mac) {
        return std::nullopt;
    }
    char mac_text[18] { };
    std::snprintf(
        mac_text,
        sizeof(mac_text),
        "%02x:%02x:%02x:%02x:%02x:%02x",
        (*mac)[0],
        (*mac)[1],
        (*mac)[2],
        (*mac)[3],
        (*mac)[4],
        (*mac)[5]);
    OpackValue info;
    info.kind = OpackValue::Kind::kDict;
    info.dict = { { OpackValue::of_string("altIRK"), OpackValue::of_bytes(options.host_alt_irk) },
                  { OpackValue::of_string("accountID"), OpackValue::of_string(options.host_identifier) },
                  { OpackValue::of_string("remotepairing_udid"), OpackValue::of_string(options.host_udid) },
                  { OpackValue::of_string("name"), OpackValue::of_string(options.host_name) },
                  { OpackValue::of_string("model"), OpackValue::of_string(options.host_model) },
                  { OpackValue::of_string("mac"), OpackValue::of_bytes(*mac) },
                  { OpackValue::of_string("btAddr"), OpackValue::of_string(mac_text) },
                  { OpackValue::of_string("remotepairing_serial_number"), OpackValue::of_string(metadata->serial_number) } };
    Bytes encoded_info;
    if (!opack_encode(info, encoded_info, err)) {
        return std::nullopt;
    }
    const auto identity = tlv_build(
        { { TlvType::Identifier, host_id },
          { TlvType::PublicKey, options.host_public_key },
          { TlvType::Signature, *signature },
          { TlvType::Info, encoded_info } });
    constexpr char nonce6[] = "\x00\x00\x00\x00PS-Msg06";
    const auto sealed6 = chacha_seal(sv(*setup_key), std::string_view(nonce6, sizeof(nonce6) - 1), identity, err);
    if (!sealed6 || !conversation.send_pairing(tlv_build({ { TlvType::State, Bytes { 6 } }, { TlvType::EncryptedData, *sealed6 } }), err)) {
        return std::nullopt;
    }
    result->record.host_identifier = options.host_identifier;
    result->record.host_private_key = options.host_private_key;
    result->record.host_public_key = options.host_public_key;
    err.clear();
    return result;
}

} // namespace maa::ios::wifi
