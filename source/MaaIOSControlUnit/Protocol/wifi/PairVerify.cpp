#include "wifi/PairVerify.h"
#include "i18n/Translation.h"

#include "wifi/PairingIdentity.h"
#include "wifi/Tlv.h"

namespace maa::ios::wifi
{

PairVerifyResult pair_verify(Rppairing& channel, const PairRecord& host, std::string& err, bool announce_failure)
{
    PairVerifyResult result;
    if (!host.complete()) {
        err = MAA_IOS_TR("Pairing record incomplete; cannot sign");
        result.outcome = VerifyOutcome::NotPaired;
        result.error = err;
        return result;
    }
    if (!host.has_peer_identity()) {
        err = MAA_IOS_TR("Pairing record has no device identity; pair again over USB");
        result.outcome = VerifyOutcome::NotPaired;
        result.error = err;
        return result;
    }

    // 1) 以 attemptPairVerify=true 发送明文 handshake，选择验证已有配对的流程。
    const json::Value host_options = j_obj({ { "attemptPairVerify", j_bool(true) } });
    const json::Value handshake_body = j_obj({ { "hostOptions", host_options }, { "wireProtocolVersion", j_int(kWireProtocolVersion) } });
    const json::Value request =
        j_obj({ { "request", j_obj({ { "_0", j_obj({ { "handshake", j_obj({ { "_0", handshake_body } }) } }) } }) } });
    const std::optional<json::Value> handshake = channel.plain_roundtrip(request, err);
    if (!handshake) {
        result.error = err;
        return result;
    }
    if (const json::Value* response = json::find(*handshake, "response")) {
        const json::Value* one = json::find(*response, "_1");
        const json::Value* hs = one != nullptr ? json::find(*one, "handshake") : nullptr;
        const json::Value* zero = hs != nullptr ? json::find(*hs, "_0") : nullptr;
        if (zero != nullptr) {
            result.device_handshake = *zero;
        }
    }

    // 2) PV-Msg01：为本次验证新建临时 X25519 密钥对，发送公钥；私钥留在本地用于共享秘密。
    const std::optional<X25519KeyPair> keypair = x25519_keypair(err);
    if (!keypair) {
        result.error = err;
        return result;
    }
    const Bytes our_pub(keypair->pub.begin(), keypair->pub.end());
    const Bytes first = tlv_build({ { TlvType::State, Bytes { 0x01 } }, { TlvType::PublicKey, our_pub } });
    const std::optional<Bytes> reply1 = pairing_data_roundtrip(channel, first, "verifyManualPairing", true, err);
    if (!reply1) {
        result.error = err;
        return result;
    }

    // 3) PV-Msg02：从临时密钥派生解密密钥，再用 USB 配对时保存的长期公钥
    // 校验设备签名。身份校验通过后才发送主机签名；不能从这次网络回复学习新公钥。
    std::string tlv_err;
    const std::map<uint8_t, Bytes> second = tlv_parse(*reply1, tlv_err);
    if (!tlv_err.empty()) {
        err = MAA_IOS_TR("PV-Msg02 TLV decode incomplete: ") + tlv_err;
        result.error = err;
        return result;
    }
    // M2 已含 Error 时不再发送 PV-Msg03，返回 NotPaired，并按选项尽力通知失败。
    // 已测设备在此继续接收 Msg03 会关闭连接，兼容约束见 docs §25.6。
    if (tlv_get(second, TlvType::Error) != nullptr) {
        if (announce_failure) {
            std::string ignored;
            json::Value body = j_obj({ { "pairVerifyFailed", j_obj({ }) } });
            channel.send_plain(j_obj({ { "event", j_obj({ { "_0", std::move(body) } }) } }), ignored);
        }
        err = MAA_IOS_TR("Device does not recognize this pairing record (not paired or pairing removed)");
        result.outcome = VerifyOutcome::NotPaired;
        result.error = err;
        return result;
    }
    const Bytes* second_state = tlv_get(second, TlvType::State);
    if (second_state == nullptr || second_state->size() != 1 || (*second_state)[0] != 0x02) {
        err = MAA_IOS_TR("PV-Msg02 response has invalid State (expected 2)");
        result.error = err;
        return result;
    }
    const Bytes* peer_pub = tlv_get(second, TlvType::PublicKey);
    if (peer_pub == nullptr || peer_pub->size() != 32) {
        err = MAA_IOS_TR("PV-Msg02 missing 32-byte public key");
        result.error = err;
        return result;
    }
    const std::optional<Bytes> shared = x25519_shared(keypair->priv, sv(*peer_pub), err);
    if (!shared) {
        result.error = err;
        return result;
    }
    const std::optional<Bytes> verify_key = hkdf_sha512(*shared, "Pair-Verify-Encrypt-Salt", "Pair-Verify-Encrypt-Info", 32, err);
    if (!verify_key) {
        result.error = err;
        return result;
    }
    const Bytes* encrypted_identity = tlv_get(second, TlvType::EncryptedData);
    if (encrypted_identity == nullptr || encrypted_identity->empty()) {
        err = MAA_IOS_TR("PV-Msg02 missing encrypted device identity");
        result.error = err;
        return result;
    }
    static constexpr char kPvMsg02[] = "\x00\x00\x00\x00PV-Msg02";
    const auto peer_identity = chacha_open(sv(*verify_key), std::string_view(kPvMsg02, sizeof(kPvMsg02) - 1), *encrypted_identity, err);
    if (!peer_identity) {
        err = MAA_IOS_TR("PV-Msg02 device identity decryption failed: ") + err;
        result.error = err;
        return result;
    }
    if (!authenticate_verify_identity(bytes_of(host.peer_identifier), host.peer_public_key, *peer_pub, our_pub, *peer_identity, err)) {
        result.error = err;
        return result;
    }
    Bytes sign_buf = our_pub;
    sign_buf.insert(sign_buf.end(), host.host_identifier.begin(), host.host_identifier.end());
    sign_buf.insert(sign_buf.end(), peer_pub->begin(), peer_pub->end());
    const std::optional<Bytes> signature = ed25519_sign(sv(host.host_private_key), sign_buf, err);
    if (!signature) {
        result.error = err;
        return result;
    }
    const Bytes identity = tlv_build({ { TlvType::Identifier, bytes_of(host.host_identifier) }, { TlvType::Signature, *signature } });
    static constexpr char kPvMsg03[] = "\x00\x00\x00\x00PV-Msg03";
    const std::string_view msg03_nonce(kPvMsg03, sizeof(kPvMsg03) - 1);
    const std::optional<Bytes> sealed = chacha_seal(sv(*verify_key), msg03_nonce, identity, err);
    if (!sealed) {
        result.error = err;
        return result;
    }

    // 4) PV-Msg03 → PV-Msg04：提交加密的主机标识和签名，检查设备是否返回 Error。
    const Bytes third = tlv_build({ { TlvType::State, Bytes { 0x03 } }, { TlvType::EncryptedData, *sealed } });
    const std::optional<Bytes> reply3 = pairing_data_roundtrip(channel, third, "verifyManualPairing", false, err);
    if (!reply3) {
        result.error = err;
        return result;
    }
    std::string final_err;
    // 完整解码后区分设备拒绝与成功状态；只有精确的单字节 State=4 才安装主密钥。
    const std::map<uint8_t, Bytes> final_fields = tlv_parse(*reply3, final_err);
    if (!final_err.empty()) {
        err = MAA_IOS_TR("PV-Msg04 TLV decode incomplete: ") + final_err;
        result.error = err;
        return result;
    }
    if (tlv_get(final_fields, TlvType::Error) != nullptr) {
        // 设备返回配对错误，按选项发送 pairVerifyFailed 并返回 NotPaired。
        // 调用方可据此检查本地记录与设备信任状态，不应将此结果等同于网络超时。
        if (announce_failure) {
            std::string ignored;
            json::Value body = j_obj({ { "pairVerifyFailed", j_obj({ }) } });
            channel.send_plain(j_obj({ { "event", j_obj({ { "_0", std::move(body) } }) } }), ignored);
        }
        err = MAA_IOS_TR("Device does not recognize this pairing record (not paired or pairing removed)");
        result.outcome = VerifyOutcome::NotPaired;
        result.error = err;
        return result;
    }
    const Bytes* final_state = tlv_get(final_fields, TlvType::State);
    if (final_state == nullptr || final_state->size() != 1 || (*final_state)[0] != 0x04) {
        err = MAA_IOS_TR("PV-Msg04 response has invalid State (expected 4)");
        result.error = err;
        return result;
    }

    // 5) 从本次 X25519 共享秘密派生双向主密钥，salt 为空，info 分别为
    // ClientEncrypt-main 与 ServerEncrypt-main；与 Pair-Verify 加密密钥的参数不同。
    const std::optional<Bytes> client_key = hkdf_sha512(*shared, "", "ClientEncrypt-main", 32, err);
    if (!client_key) {
        result.error = err;
        return result;
    }
    const std::optional<Bytes> server_key = hkdf_sha512(*shared, "", "ServerEncrypt-main", 32, err);
    if (!server_key) {
        result.error = err;
        return result;
    }
    channel.install_main_keys(*client_key, *server_key);
    result.shared_secret = *shared;
    err.clear();
    result.outcome = VerifyOutcome::Paired;
    return result;
}

} // namespace maa::ios::wifi
