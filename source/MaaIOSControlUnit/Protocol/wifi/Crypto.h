#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace maa::ios::wifi
{

using Bytes = std::vector<uint8_t>;

/// 字节缓冲区的只读视图，保留完整长度及内嵌 NUL。视图不拥有数据。
[[nodiscard]] inline std::string_view sv(const Bytes& data)
{
    return std::string_view(reinterpret_cast<const char*>(data.data()), data.size()); // NOLINT
}

/// 将文本复制为字节，供签名消息拼接 identifier 等字段。
[[nodiscard]] inline Bytes bytes_of(std::string_view text)
{
    return Bytes(text.begin(), text.end());
}

/// X25519 临时密钥对；每次 PairVerify 使用新生成的密钥对。
struct X25519KeyPair
{
    std::array<uint8_t, 32> priv { };
    std::array<uint8_t, 32> pub { };
};

std::optional<X25519KeyPair> x25519_keypair(std::string& err);

/// Ed25519 主机身份密钥对；PairSetup 时生成，随后保存在配对记录中。
struct Ed25519KeyPair
{
    std::array<uint8_t, 32> seed { }; ///< 私钥种子（不是 PEM/DER）
    std::array<uint8_t, 32> pub { };
};

std::optional<Ed25519KeyPair> ed25519_keypair(std::string& err);

/// CSPRNG 字节。`n` 为 0 时返回空 vector。
std::optional<Bytes> random_bytes(size_t n, std::string& err);

/// 计算 X25519 共享密钥。对端公钥必须是 32 字节；拒绝低阶点和全零共享密钥，
/// 避免后续密钥从可预测的值派生。校验或计算失败时返回 nullopt。
std::optional<Bytes> x25519_shared(const std::array<uint8_t, 32>& priv, std::string_view peer_pub, std::string& err);

/// Ed25519 签名（64 字节）。`seed` 是 32 字节的私钥种子，不是 PEM、不是 DER。
std::optional<Bytes> ed25519_sign(std::string_view seed, const Bytes& msg, std::string& err);

/// 校验 Ed25519 签名：公钥为 32 字节、签名为 64 字节，均为原始字节格式。
/// msg 是完整消息，允许为空；使用 PureEd25519，不做预哈希。
/// 成功时清空 err；长度错误、签名不匹配或 OpenSSL 失败时返回 false 并填写原因。
bool ed25519_verify(std::string_view public_key, const Bytes& msg, const Bytes& signature, std::string& err);

/// OpenSSL HKDF-SHA512。空 salt 与 RFC 5869 的全零盐等价。
/// 输出最多 255 * 64 字节，info 最多 1024 字节，输入需可表示为 int 长度。
/// 成功时清空 err；零长度输出返回空 Bytes。
std::optional<Bytes> hkdf_sha512(const Bytes& ikm, std::string_view salt, std::string_view info, size_t out_len, std::string& err);

/// ChaCha20-Poly1305-IETF（12 字节 nonce + 16 字节标签）。`nonce` 必须正好 12 字节。
std::optional<Bytes> chacha_seal(std::string_view key, std::string_view nonce, const Bytes& plain, std::string& err);
std::optional<Bytes> chacha_open(std::string_view key, std::string_view nonce, const Bytes& sealed, std::string& err);

std::string b64_encode(const Bytes& data);
std::string b64_encode(std::string_view data);
std::optional<Bytes> b64_decode(std::string_view text, std::string& err);

} // namespace maa::ios::wifi
