#pragma once

#include <optional>
#include <string>

#include "wifi/Crypto.h"

namespace maa::ios::wifi
{

/// 已通过本次 PairSetup M6 签名校验的设备长期身份。
/// 标识保留原始字节；info 是可选的原始 OPACK 数据，供调用方提取 altIRK。
struct PairingIdentity
{
    Bytes identifier;
    Bytes public_key;
    Bytes info;
};

/// 校验已认证解密的 M6 内层 TLV。调用方负责外层阶段和 ChaCha20-Poly1305 校验。
/// 签名绑定 SRP 会话派生前缀、设备标识和设备长期公钥；成功时清空 err。
std::optional<PairingIdentity>
    authenticate_setup_identity(const Bytes& srp_session_key, const Bytes& decrypted_plain_tlv, std::string& err);

/// 校验已认证解密的 M2 内层 TLV，并与保存的设备标识、长期公钥匹配。
/// 签名绑定设备临时公钥、设备原始标识和主机临时公钥；成功时清空 err。
/// M2 无需重复携带长期公钥；若提供，则必须与 trusted_long_term_key 相同。
bool authenticate_verify_identity(
    const Bytes& trusted_identifier,
    const Bytes& trusted_long_term_key,
    const Bytes& peer_ephemeral,
    const Bytes& host_ephemeral,
    const Bytes& decrypted_plain_tlv,
    std::string& err);

} // namespace maa::ios::wifi
