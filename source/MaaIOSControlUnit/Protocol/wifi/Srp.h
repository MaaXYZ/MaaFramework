#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wifi/Crypto.h"

namespace maa::ios::wifi
{

/// Apple pair-setup 使用的 SRP-6a 客户端（3072 位模数、SHA-512）。
/// 大整数运算和摘要使用 OpenSSL；下面的编码规则是设备协议适配的一部分。
///
/// 已有配对实现及离线向量验证的规则：
///  - 整数进哈希前使用最小大端字节，不添加符号字节；
///    只有 u = H(PAD(A)|PAD(B)) 与 k = H(N|PAD(g)) 用 384 字节定长 PAD。
///  - x = H(s | H(I ":" P))，内层是字节、外层把 salt 原字节拼在前面。
///  - M1 = H( (H(N) xor H(g)) | H(I) | s | A | B | K )，M2 = H(A | M1 | K)。
class SrpClient
{
public:
    /// private_hex 仅供测试注入正整数私钥；空则生成随机 1024 位私钥。
    SrpClient(std::string user, std::string password, std::string private_hex = "");

    /// 输入设备 pair-setup M2 的 salt 与 B。B 必须为 1..384 字节且 B % N != 0。
    /// 只有全部计算成功后才发布 A、K、M1、M2；失败时清除已有结果并通过 err 报告。
    bool process(const Bytes& salt, const Bytes& server_public, std::string& err);

    [[nodiscard]] const Bytes& client_public() const { return a_public_; } // A

    [[nodiscard]] const Bytes& session_key() const { return k_; }          // K

    [[nodiscard]] const Bytes& client_proof() const { return m1_; }        // M1

    /// 计算成功后校验设备的 64 字节证明 M2；未计算或失败后不接受任何证明。
    [[nodiscard]] bool verify_server_proof(const Bytes& m2) const;

private:
    std::string user_;
    std::string password_;
    std::string private_hex_;
    Bytes a_public_;
    Bytes k_;
    Bytes m1_;
    Bytes m2_;
};

/// 同一 Apple SRP-6a 编码的服务端；每个实例由单个配对会话顺序调用。
/// 仅验证本次 SRP 证明，不赋予首次配对入口或设备身份任何额外信任。
class SrpServer
{
public:
    /// private_hex 仅供测试注入正整数 b；空则生成随机 1024 位私钥。
    SrpServer(std::string user, std::string password, std::string private_hex = "");
    ~SrpServer();
    SrpServer(const SrpServer&) = delete;
    SrpServer& operator=(const SrpServer&) = delete;

    /// 启动或重启一个 challenge，清除旧结果。salt 长度必须为 1..255 字节。
    /// 成功后仅发布 B；还不发布会话密钥或服务端证明。
    bool initialize(const Bytes& salt, std::string& err);

    [[nodiscard]] const Bytes& server_public() const { return b_public_; } // B

    /// 每个 challenge 只接受一次 A/M1。A 长度为 1..384 字节且 A % N != 0。
    /// 仅在常量时间校验 64 字节 M1 成功后发布 K/M2。
    /// 失败、未初始化或重复调用都清除 K/M2；须 initialize 才能重试。
    bool process(const Bytes& client_public, const Bytes& client_proof, std::string& err);

    [[nodiscard]] const Bytes& session_key() const { return k_; }   // K

    [[nodiscard]] const Bytes& server_proof() const { return m2_; } // M2

private:
    void clear_secrets();
    std::string user_;
    std::string password_;
    std::string private_hex_;
    Bytes salt_;
    Bytes b_public_;
    Bytes b_private_;
    Bytes verifier_;
    Bytes k_;
    Bytes m2_;
    bool awaiting_client_ = false;
};

} // namespace maa::ios::wifi
