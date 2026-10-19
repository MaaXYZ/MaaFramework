#include "wifi/PairSetup.h"

#include <openssl/evp.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

#include <cstdio>
#include <map>

#include "wifi/HostMetadata.h"
#include "wifi/Opack.h"
#include "wifi/PairVerify.h"
#include "wifi/PairingIdentity.h"
#include "wifi/Srp.h"
#include "wifi/Tlv.h"

namespace maa::ios::wifi
{
namespace
{

/// RFC 4122 的 DNS 命名空间 UUID（`6ba7b810-9dad-11d1-80b4-00c04fd430c8`）。
constexpr uint8_t kNamespaceDns[16] = { 0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };

std::string hex_upper(const uint8_t* data, size_t len)
{
    static constexpr char kDigits[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out.push_back(kDigits[data[i] >> 4]);
        out.push_back(kDigits[data[i] & 0x0F]);
    }
    return out;
}

/// UUID v3 所需的 MD5 摘要，仅用于确定性标识生成，不用于认证或签名。
/// 更换摘要算法会改变生成标识，无法替代已有记录中注册的 identifier。
bool md5_of(std::string_view data, Bytes& out, std::string& err)
{
    out.assign(16, 0);
    unsigned int len = 0;
    if (EVP_Digest(data.data(), data.size(), out.data(), &len, EVP_md5(), nullptr) != 1 || len != 16) {
        err = "MD5 unavailable; check OpenSSL algorithm providers";
        return false;
    }
    return true;
}

/// 发送 handshake，attemptPairVerify 由调用方选择。要求响应包含完整的
/// response._1.handshake._0 路径；peerDeviceInfo.identifier 用于保存广播标识。
std::optional<json::Value> do_handshake(Rppairing& channel, bool attempt_verify, std::string& err)
{
    const json::Value host_options = j_obj({ { "attemptPairVerify", j_bool(attempt_verify) } });
    const json::Value body = j_obj({ { "hostOptions", host_options }, { "wireProtocolVersion", j_int(kWireProtocolVersion) } });
    const json::Value request = j_obj({ { "request", j_obj({ { "_0", j_obj({ { "handshake", j_obj({ { "_0", body } }) } }) } }) } });
    const std::optional<json::Value> reply = channel.plain_roundtrip(request, err);
    if (!reply) {
        return std::nullopt;
    }
    const json::Value* response = json::find(*reply, "response");
    const json::Value* one = response != nullptr ? json::find(*response, "_1") : nullptr;
    const json::Value* handshake = one != nullptr ? json::find(*one, "handshake") : nullptr;
    const json::Value* zero = handshake != nullptr ? json::find(*handshake, "_0") : nullptr;
    if (zero == nullptr) {
        err = "Handshake response missing response._1.handshake._0";
        return std::nullopt;
    }
    return std::optional<json::Value>(std::in_place, *zero);
}

/// 解析完整外层 TLV，成功回复的 State 必须是预期的单字节阶段。
/// setup 的 Error 直接失败；verify 探测可显式允许 Error，由调用方通知并继续 setup。
/// 无论是否允许 Error，都不能忽略不完整编码。本函数不解密 EncryptedData。
std::optional<std::map<uint8_t, Bytes>>
    parse_reply(const Bytes& raw, const char* which, uint8_t expected_state, std::string& err, bool allow_error = false)
{
    std::string tlv_err;
    std::map<uint8_t, Bytes> fields = tlv_parse(raw, tlv_err);
    if (!tlv_err.empty()) {
        err = std::string(which) + " TLV decode incomplete: " + tlv_err;
        return std::nullopt;
    }
    if (const Bytes* code = tlv_get(fields, TlvType::Error)) {
        if (allow_error) {
            return fields;
        }
        err = std::string(which) + " returned error code 0x";
        err += hex_upper(code->data(), code->size());
        return std::nullopt;
    }
    const Bytes* state = tlv_get(fields, TlvType::State);
    if (state == nullptr || state->size() != 1 || (*state)[0] != expected_state) {
        err = std::string(which) + " reply must contain a one-byte State equal to " + std::to_string(expected_state);
        return std::nullopt;
    }
    return fields;
}

/// 在发送 pairingData 前检查设备是否明确声明 allowsPairSetup=false。
/// 字段缺失或非布尔值时当前实现继续尝试，不将其当作已确认允许配对。
bool plane_allows_pair_setup(const json::Value& handshake, std::string& err)
{
    const json::Value* options = json::find(handshake, "deviceOptions");
    const json::Value* allowed = options != nullptr ? json::find(*options, "allowsPairSetup") : nullptr;
    if (allowed != nullptr && allowed->is_boolean() && !allowed->as_boolean()) {
        err =
            "Control channel does not allow pair-setup "
            "(deviceOptions.allowsPairSetup=false). Tested iOS 27 devices require the "
            "RemoteXPC entry point (wifi_probe --pair-setup-xpc)";
        return false;
    }
    return true;
}

} // namespace

std::string host_identifier_uuid3(std::string_view hostname)
{
    Bytes input(kNamespaceDns, kNamespaceDns + sizeof(kNamespaceDns));
    input.insert(input.end(), hostname.begin(), hostname.end());
    Bytes digest;
    std::string err;
    if (!md5_of(sv(input), digest, err)) {
        return { };
    }
    digest[6] = static_cast<uint8_t>((digest[6] & 0x0F) | 0x30); // UUID 版本 3
    digest[8] = static_cast<uint8_t>((digest[8] & 0x3F) | 0x80); // RFC 4122 变体位
    std::string out = hex_upper(digest.data(), 4) + "-" + hex_upper(digest.data() + 4, 2) + "-" + hex_upper(digest.data() + 6, 2) + "-"
                      + hex_upper(digest.data() + 8, 2) + "-" + hex_upper(digest.data() + 10, 6);
    return out;
}

std::string local_hostname()
{
    char buf[256] = { 0 };
    if (::gethostname(buf, sizeof(buf) - 1) != 0) {
        return { };
    }
    return std::string(buf);
}

PairSetupResult pair_setup(
    Rppairing& channel,
    std::string_view host_identifier,
    std::string_view hostname,
    std::string_view udid,
    const ProgressFn& progress,
    const PairSetupOptions& options,
    std::string& err)
{
    PairSetupResult result;
    const auto fail = [&result, &err]() {
        result.error = err;
        return result;
    };
    if (host_identifier.empty()) {
        err = "Host identifier missing; provide one if hostname lookup is unavailable";
        return fail();
    }
    const auto metadata = host_metadata(host_identifier, err);
    if (!metadata) {
        return fail();
    }
    // 本次 setup 生成一组 Ed25519 主机密钥，verify 探测签名与 M5 注册共用此密钥。
    // 记录保存同一密钥的种子及公钥，供后续连接签名。
    const std::optional<Ed25519KeyPair> host_key = ed25519_keypair(err);
    if (!host_key) {
        return fail();
    }
    const Bytes host_public(host_key->pub.begin(), host_key->pub.end());
    // pairingData 的主机显示名去除末尾 .local，与已有客户端样本保持一致。
    // 这里只调整显示名，不修改 host_identifier。
    std::string host_label(hostname);
    if (host_label.size() > 6 && host_label.compare(host_label.size() - 6, 6, ".local") == 0) {
        host_label.erase(host_label.size() - 6);
    }

    // 1) 根据选项先执行 verify 探测，或直接以 attemptPairVerify=false 握手。
    // 两条路径都在任何 pairingData 前检查 allowsPairSetup。
    if (options.probe_verify_first) {
        const std::optional<json::Value> device_handshake = do_handshake(channel, /*attempt_verify=*/true, err);
        if (!device_handshake) {
            return fail();
        }
        result.device_handshake = *device_handshake;
        if (!plane_allows_pair_setup(*device_handshake, err)) {
            return fail();
        }
        // verify 探测使用本次生成的主机密钥签名，不用占位签名字节。
        // 无效签名可能使设备停留在 verify 状态，影响后续 upgrade。
        const std::optional<X25519KeyPair> vk = x25519_keypair(err);
        if (!vk) {
            return fail();
        }
        const Bytes our_x_pub(vk->pub.begin(), vk->pub.end());
        const Bytes v1 = tlv_build({ { TlvType::State, Bytes { 0x01 } }, { TlvType::PublicKey, our_x_pub } });
        const std::optional<Bytes> raw_v2 = pairing_data_roundtrip(channel, v1, "verifyManualPairing", true, err);
        if (!raw_v2) {
            return fail();
        }
        const std::optional<std::map<uint8_t, Bytes>> vf = parse_reply(*raw_v2, "verify-M2", 2, err, true);
        if (!vf) {
            return fail();
        }
        const auto send_verify_failed = [&channel]() {
            std::string ignored;
            json::Value body = j_obj({ { "pairVerifyFailed", j_obj({ }) } });
            channel.send_plain(j_obj({ { "event", j_obj({ { "_0", std::move(body) } }) } }), ignored);
        };
        if (tlv_get(*vf, TlvType::Error) != nullptr) {
            // M2 已含 Error 时结束 verify 探测，不再发送 Msg03。
            // 尽力通知 pairVerifyFailed 后继续 setup。
            send_verify_failed();
        }
        else {
            // M2 未含 Error，按公钥派生 verify 密钥并发送 Msg03，检查 M4 是否拒绝。
            const Bytes* peer_x = tlv_get(*vf, TlvType::PublicKey);
            if (peer_x == nullptr || peer_x->size() != 32) {
                err = "Verify M2 missing 32-byte public key";
                return fail();
            }
            const std::optional<Bytes> shared = x25519_shared(vk->priv, sv(*peer_x), err);
            if (!shared) {
                return fail();
            }
            const std::optional<Bytes> vkey = hkdf_sha512(*shared, "Pair-Verify-Encrypt-Salt", "Pair-Verify-Encrypt-Info", 32, err);
            if (!vkey) {
                return fail();
            }
            Bytes signbuf = our_x_pub;
            signbuf.insert(signbuf.end(), host_identifier.begin(), host_identifier.end());
            signbuf.insert(signbuf.end(), peer_x->begin(), peer_x->end());
            const std::optional<Bytes> sig = ed25519_sign(
                std::string_view(
                    reinterpret_cast<const char*>(host_key->seed.data()), // NOLINT
                    host_key->seed.size()),
                signbuf,
                err);
            if (!sig) {
                return fail();
            }
            const Bytes identity = tlv_build({ { TlvType::Identifier, bytes_of(host_identifier) }, { TlvType::Signature, *sig } });
            static constexpr char kPvMsg03[] = "\x00\x00\x00\x00PV-Msg03";
            const std::optional<Bytes> sealed = chacha_seal(sv(*vkey), std::string_view(kPvMsg03, sizeof(kPvMsg03) - 1), identity, err);
            if (!sealed) {
                return fail();
            }
            const Bytes v3 = tlv_build({ { TlvType::State, Bytes { 0x03 } }, { TlvType::EncryptedData, *sealed } });
            const std::optional<Bytes> raw_v4 = pairing_data_roundtrip(channel, v3, "verifyManualPairing", false, err);
            if (!raw_v4) {
                return fail();
            }
            const std::optional<std::map<uint8_t, Bytes>> v4f = parse_reply(*raw_v4, "verify-M4", 4, err, true);
            if (!v4f) {
                return fail();
            }
            if (tlv_get(*v4f, TlvType::Error) != nullptr) {
                // 探测收到 Error 表示本次凭据未被接受，发送失败通知后继续 setup。
                send_verify_failed();
            }
            else {
                err = "Device accepted this identifier; already paired, pair-setup is unnecessary";
                return fail();
            }
        }
        if (progress) {
            progress("Verify completed without accepting this key; sending upgrade M1");
        }
    }
    else {
        const std::optional<json::Value> device_handshake = do_handshake(channel, /*attempt_verify=*/false, err);
        if (!device_handshake) {
            return fail();
        }
        result.device_handshake = *device_handshake;
        if (!plane_allows_pair_setup(*device_handshake, err)) {
            return fail();
        }
        if (progress) {
            progress("Handshake (attemptPairVerify=false) completed; sending M1");
        }
    }
    // 2) M1 → M2：设备给出 SRP 的 salt 与 B。kind 见 PairSetupOptions::pairing_kind。
    const std::string kind = options.pairing_kind;
    const Bytes m1 = tlv_build({ { TlvType::Method, Bytes { 0x00 } }, { TlvType::State, Bytes { 0x01 } } });
    const std::optional<Bytes> raw_m2 = pairing_data_roundtrip(channel, m1, kind, true, err, host_label, progress);
    if (!raw_m2) {
        return fail();
    }
    const std::optional<std::map<uint8_t, Bytes>> fields2 = parse_reply(*raw_m2, "M2", 2, err);
    if (!fields2) {
        return fail();
    }
    const Bytes* server_public = tlv_get(*fields2, TlvType::PublicKey);
    const Bytes* salt = tlv_get(*fields2, TlvType::Salt);
    if (server_public == nullptr || salt == nullptr || server_public->empty() || salt->empty()) {
        err = "M2 missing PUBLIC_KEY or SALT";
        return fail();
    }

    // 3) 以固定用户名 Pair-Setup 和 PIN "000000" 执行 SRP-6a；不支持输入其他 PIN。
    SrpClient srp("Pair-Setup", "000000");
    if (!srp.process(*salt, *server_public, err)) {
        return fail();
    }

    // 4) M3 → M4：提交 SRP 公钥 A 与客户端证明，接收并验证服务端证明。
    // SRP 证明 M1/M2 与配对消息阶段 M1/M2 是不同命名。
    const Bytes m3 = tlv_build(
        { { TlvType::State, Bytes { 0x03 } }, { TlvType::PublicKey, srp.client_public() }, { TlvType::Proof, srp.client_proof() } });
    const std::optional<Bytes> raw_m4 = pairing_data_roundtrip(channel, m3, kind, false, err, host_label, progress);
    if (!raw_m4) {
        return fail();
    }
    const std::optional<std::map<uint8_t, Bytes>> fields4 = parse_reply(*raw_m4, "M4", 4, err);
    if (!fields4) {
        return fail();
    }
    const Bytes* server_proof = tlv_get(*fields4, TlvType::Proof);
    if (server_proof == nullptr) {
        err = "M4 missing PROOF";
        return fail();
    }
    // 必须验证 SRP 服务端证明后才继续注册主机密钥；证明绑定本次 SRP 会话。
    // 固定 PIN 的证明不等于验证设备长期身份，不能据此声称完成独立设备身份认证。
    if (!srp.verify_server_proof(*server_proof)) {
        err = "Device SRP proof mismatch; pairing aborted. Check PIN and peer identity";
        return fail();
    }
    if (progress) {
        progress("SRP mutual verification passed; registering host key on device (M5)");
    }

    const Bytes& session_key = srp.session_key();
    const std::optional<Bytes> setup_key = hkdf_sha512(session_key, "Pair-Setup-Encrypt-Salt", "Pair-Setup-Encrypt-Info", 32, err);
    if (!setup_key) {
        return fail();
    }
    const std::optional<Bytes> sign_prefix =
        hkdf_sha512(session_key, "Pair-Setup-Controller-Sign-Salt", "Pair-Setup-Controller-Sign-Info", 32, err);
    if (!sign_prefix) {
        return fail();
    }
    Bytes signbuf = *sign_prefix;
    signbuf.insert(signbuf.end(), host_identifier.begin(), host_identifier.end());
    signbuf.insert(signbuf.end(), host_public.begin(), host_public.end());
    const std::optional<Bytes> signature = ed25519_sign(
        std::string_view(
            reinterpret_cast<const char*>(host_key->seed.data()), // NOLINT
            host_key->seed.size()),
        signbuf,
        err);
    if (!signature) {
        return fail();
    }

    // 5) M5 的主机信息使用 OPACK 字典。altIRK 是随机生成的主机身份解析密钥，
    // mac/btAddr 来自同一组随机六字节，不读取真实网卡地址。设备对这些字段的
    // 必需性和具体使用方式尚未确认。
    const std::optional<Bytes> our_alt_irk = random_bytes(16, err);
    if (!our_alt_irk) {
        return fail();
    }
    const std::optional<Bytes> fake_mac = random_bytes(6, err);
    if (!fake_mac) {
        return fail();
    }
    char mac_text[18] = { 0 };
    std::snprintf(
        mac_text,
        sizeof(mac_text),
        "%02x:%02x:%02x:%02x:%02x:%02x",
        (*fake_mac)[0],
        (*fake_mac)[1],
        (*fake_mac)[2],
        (*fake_mac)[3],
        (*fake_mac)[4],
        (*fake_mac)[5]);
    OpackValue info;
    info.kind = OpackValue::Kind::kDict;
    info.dict = {
        { OpackValue::of_string("altIRK"), OpackValue::of_bytes(*our_alt_irk) },
        { OpackValue::of_string("btAddr"), OpackValue::of_string(mac_text) },
        { OpackValue::of_string("mac"), OpackValue::of_bytes(*fake_mac) },
        { OpackValue::of_string("remotepairing_serial_number"), OpackValue::of_string(metadata->serial_number) },
        { OpackValue::of_string("accountID"), OpackValue::of_string(std::string(host_identifier)) },
        { OpackValue::of_string("model"), OpackValue::of_string(metadata->model) },
        { OpackValue::of_string("name"), OpackValue::of_string(host_label) },
    };
    Bytes info_bytes;
    if (!opack_encode(info, info_bytes, err)) {
        return fail();
    }
    const Bytes identity_tlv = tlv_build(
        { { TlvType::Identifier, bytes_of(host_identifier) },
          { TlvType::PublicKey, host_public },
          { TlvType::Signature, *signature },
          { TlvType::Info, info_bytes } });
    static constexpr char kPsMsg05[] = "\x00\x00\x00\x00PS-Msg05";
    const std::optional<Bytes> sealed = chacha_seal(sv(*setup_key), std::string_view(kPsMsg05, sizeof(kPsMsg05) - 1), identity_tlv, err);
    if (!sealed) {
        return fail();
    }
    const Bytes m5 = tlv_build({ { TlvType::EncryptedData, *sealed }, { TlvType::State, Bytes { 0x05 } } });
    const std::optional<Bytes> raw_m6 = pairing_data_roundtrip(channel, m5, kind, false, err, host_label, progress);
    if (!raw_m6) {
        return fail();
    }
    const std::optional<std::map<uint8_t, Bytes>> fields6 = parse_reply(*raw_m6, "M6", 6, err);
    if (!fields6) {
        return fail();
    }

    // 6) M6 必须携带设备身份密文。AEAD 解密后，用本次 SRP 会话派生的前缀
    // 校验设备 Ed25519 签名，并保存原始标识和公钥，供后续 PairVerify 认证使用。
    // 固定 PIN 的 SRP 和设备自签名仍需要可信的首次配对入口作为身份信任来源。
    const Bytes* sealed6 = tlv_get(*fields6, TlvType::EncryptedData);
    if (sealed6 == nullptr) {
        err = "PairSetup M6 missing encrypted device identity";
        return fail();
    }
    static constexpr char kPsMsg06[] = "\x00\x00\x00\x00PS-Msg06";
    std::string open_err;
    const std::optional<Bytes> plain6 = chacha_open(sv(*setup_key), std::string_view(kPsMsg06, sizeof(kPsMsg06) - 1), *sealed6, open_err);
    if (!plain6) {
        err = "Cannot decrypt M6: " + open_err;
        return fail();
    }
    const std::optional<PairingIdentity> peer_identity = authenticate_setup_identity(session_key, *plain6, err);
    if (!peer_identity) {
        return fail();
    }

    // altIRK 只用于 mDNS authTag 匹配，不参与设备签名校验。Info 缺失或其 OPACK
    // 无法解析时可以留空；设备身份字段和签名已经由上一步单独验证。
    Bytes peer_alt_irk;
    if (!peer_identity->info.empty()) {
        OpackValue parsed;
        std::string opack_err;
        if (opack_decode(peer_identity->info, parsed, opack_err)) {
            if (const OpackValue* irk = parsed.find("altIRK")) {
                if (irk->kind == OpackValue::Kind::kBytes && irk->bytes.size() == 16) {
                    peer_alt_irk = irk->bytes;
                }
            }
        }
    }

    // 7) 从 SRP 会话密钥派生双向控制面主密钥，salt 为空，info 分别为
    // ClientEncrypt-main 与 ServerEncrypt-main；与 setup 加密及签名的派生参数不同。
    const std::optional<Bytes> client_key = hkdf_sha512(session_key, "", "ClientEncrypt-main", 32, err);
    if (!client_key) {
        return fail();
    }
    const std::optional<Bytes> server_key = hkdf_sha512(session_key, "", "ServerEncrypt-main", 32, err);
    if (!server_key) {
        return fail();
    }
    channel.install_main_keys(*client_key, *server_key);

    PairRecord record;
    record.udid = std::string(udid);
    record.host_identifier = std::string(host_identifier);
    record.peer_identifier.assign(peer_identity->identifier.begin(), peer_identity->identifier.end());
    record.host_private_key.assign(host_key->seed.begin(), host_key->seed.end());
    record.host_public_key = host_public;
    record.peer_public_key = peer_identity->public_key;
    record.peer_alt_irk = peer_alt_irk;

    result.record = std::move(record);
    result.ok = true;
    return result;
}

} // namespace maa::ios::wifi
