#include "wifi/PairingIdentity.h"

#include "i18n/Translation.h"
#include "wifi/Tlv.h"

#include <array>
#include <map>

namespace maa::ios::wifi
{
namespace
{

using IdentityFields = std::map<uint8_t, Bytes>;

int identity_field_index(uint8_t type)
{
    switch (static_cast<TlvType>(type)) {
    case TlvType::Identifier:
        return 0;
    case TlvType::PublicKey:
        return 1;
    case TlvType::Signature:
        return 2;
    default:
        return -1;
    }
}

std::optional<IdentityFields> parse_identity(const Bytes& plain, std::string& err)
{
    std::string parse_err;
    auto fields = tlv_parse(plain, parse_err);
    if (!parse_err.empty()) {
        err = MAA_IOS_TR("Identity TLV decode failed: ") + parse_err;
        return std::nullopt;
    }

    // tlv_parse 已检查完整编码及长度。这里只补充三个身份字段的重复项约束，
    // 防止解析器将多条独立标识或密钥静默拼接；Info 与未知字段保留现有语义。
    constexpr std::array<const char*, 3> names { "Identifier", "PublicKey", "Signature" };
    std::array<bool, 3> seen { };
    uint8_t previous_type = 0;
    size_t previous_length = 0;
    for (size_t offset = 0; offset < plain.size();) {
        const uint8_t type = plain[offset];
        const size_t length = plain[offset + 1];
        const int index = identity_field_index(type);
        if (index >= 0) {
            const auto field = static_cast<size_t>(index);
            if (seen[field] && (previous_type != type || previous_length != 255 || length == 0)) {
                err = MAA_IOS_TR("Identity TLV field repeated without a valid continuation: ") + std::string(names[field]);
                return std::nullopt;
            }
            seen[field] = true;
        }
        previous_type = type;
        previous_length = length;
        offset += 2 + length;
    }
    return fields;
}

} // namespace

std::optional<PairingIdentity> authenticate_setup_identity(const Bytes& srp_session_key, const Bytes& decrypted_plain_tlv, std::string& err)
{
    if (srp_session_key.size() != 64) {
        err = MAA_IOS_TR("PS-Msg06 identity validation requires a 64-byte SRP session key");
        return std::nullopt;
    }
    std::string inner_err;
    const auto fields = parse_identity(decrypted_plain_tlv, inner_err);
    if (!fields) {
        err = MAA_IOS_TR("PS-Msg06 identity validation failed: ") + inner_err;
        return std::nullopt;
    }
    const Bytes* identifier = tlv_get(*fields, TlvType::Identifier);
    if (identifier == nullptr || identifier->empty()) {
        err = MAA_IOS_TR("PS-Msg06 missing nonempty device Identifier");
        return std::nullopt;
    }
    const Bytes* public_key = tlv_get(*fields, TlvType::PublicKey);
    if (public_key == nullptr || public_key->size() != 32) {
        err = MAA_IOS_TR("PS-Msg06 missing 32-byte device public key");
        return std::nullopt;
    }
    const Bytes* signature = tlv_get(*fields, TlvType::Signature);
    if (signature == nullptr || signature->size() != 64) {
        err = MAA_IOS_TR("PS-Msg06 missing 64-byte device signature");
        return std::nullopt;
    }
    const auto prefix = hkdf_sha512(srp_session_key, "Pair-Setup-Accessory-Sign-Salt", "Pair-Setup-Accessory-Sign-Info", 32, inner_err);
    if (!prefix) {
        err = MAA_IOS_TR("PS-Msg06 signature key derivation failed: ") + inner_err;
        return std::nullopt;
    }
    Bytes message = *prefix;
    message.insert(message.end(), identifier->begin(), identifier->end());
    message.insert(message.end(), public_key->begin(), public_key->end());
    if (!ed25519_verify(sv(*public_key), message, *signature, inner_err)) {
        err = MAA_IOS_TR("PS-Msg06 device signature verification failed: ") + inner_err;
        return std::nullopt;
    }
    PairingIdentity identity;
    identity.identifier = *identifier;
    identity.public_key = *public_key;
    if (const Bytes* info = tlv_get(*fields, TlvType::Info)) {
        identity.info = *info;
    }
    err.clear();
    return identity;
}

bool authenticate_verify_identity(
    const Bytes& trusted_identifier,
    const Bytes& trusted_long_term_key,
    const Bytes& peer_ephemeral,
    const Bytes& host_ephemeral,
    const Bytes& decrypted_plain_tlv,
    std::string& err)
{
    if (trusted_identifier.empty()) {
        err = MAA_IOS_TR("PV-Msg02 requires a saved device Identifier; pair again over USB");
        return false;
    }
    if (trusted_long_term_key.size() != 32) {
        err = MAA_IOS_TR("PV-Msg02 requires a saved 32-byte device public key; pair again over USB");
        return false;
    }
    if (peer_ephemeral.size() != 32 || host_ephemeral.size() != 32) {
        err = MAA_IOS_TR("PV-Msg02 identity validation requires 32-byte device and host ephemeral keys");
        return false;
    }
    std::string inner_err;
    const auto fields = parse_identity(decrypted_plain_tlv, inner_err);
    if (!fields) {
        err = MAA_IOS_TR("PV-Msg02 identity validation failed: ") + inner_err;
        return false;
    }
    const Bytes* identifier = tlv_get(*fields, TlvType::Identifier);
    if (identifier == nullptr || identifier->empty()) {
        err = MAA_IOS_TR("PV-Msg02 missing nonempty device Identifier");
        return false;
    }
    if (*identifier != trusted_identifier) {
        err = MAA_IOS_TR("PV-Msg02 device Identifier does not match the pairing record");
        return false;
    }
    if (const Bytes* public_key = tlv_get(*fields, TlvType::PublicKey)) {
        if (public_key->size() != 32 || *public_key != trusted_long_term_key) {
            err = MAA_IOS_TR("PV-Msg02 device public key does not match the pairing record");
            return false;
        }
    }
    const Bytes* signature = tlv_get(*fields, TlvType::Signature);
    if (signature == nullptr || signature->size() != 64) {
        err = MAA_IOS_TR("PV-Msg02 missing 64-byte device signature");
        return false;
    }
    Bytes message = peer_ephemeral;
    message.insert(message.end(), identifier->begin(), identifier->end());
    message.insert(message.end(), host_ephemeral.begin(), host_ephemeral.end());
    if (!ed25519_verify(sv(trusted_long_term_key), message, *signature, inner_err)) {
        err = MAA_IOS_TR("PV-Msg02 device signature verification failed: ") + inner_err;
        return false;
    }
    err.clear();
    return true;
}

} // namespace maa::ios::wifi
