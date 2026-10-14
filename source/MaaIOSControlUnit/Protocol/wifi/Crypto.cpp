#include "wifi/Crypto.h"
#include "i18n/Translation.h"
#include "util/Base64.h"

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>

namespace maa::ios::wifi
{
namespace
{

struct EvpPkeyDeleter
{
    void operator()(EVP_PKEY* p) const { EVP_PKEY_free(p); }
};

struct EvpCtxDeleter
{
    void operator()(EVP_PKEY_CTX* p) const { EVP_PKEY_CTX_free(p); }
};

struct EvpMdCtxDeleter
{
    void operator()(EVP_MD_CTX* p) const { EVP_MD_CTX_free(p); }
};

struct CipherCtxDeleter
{
    void operator()(EVP_CIPHER_CTX* p) const { EVP_CIPHER_CTX_free(p); }
};

using PkeyUp = std::unique_ptr<EVP_PKEY, EvpPkeyDeleter>;
using PctxUp = std::unique_ptr<EVP_PKEY_CTX, EvpCtxDeleter>;
using MdCtxUp = std::unique_ptr<EVP_MD_CTX, EvpMdCtxDeleter>;
using CtxUp = std::unique_ptr<EVP_CIPHER_CTX, CipherCtxDeleter>;

const unsigned char* u8(std::string_view s)
{
    return reinterpret_cast<const unsigned char*>(s.data()); // NOLINT: 只读视图转字节
}

/// 裸密钥（32 字节，非 PEM/DER）转 EVP_PKEY。`as_private=false` 时按公钥构造。
PkeyUp raw_key(int pkey_type, std::string_view data, bool as_private, std::string& err)
{
    PkeyUp p(
        as_private ? EVP_PKEY_new_raw_private_key(pkey_type, nullptr, u8(data), static_cast<int>(data.size()))
                   : EVP_PKEY_new_raw_public_key(pkey_type, nullptr, u8(data), static_cast<int>(data.size())));
    if (!p) {
        err = as_private ? MAA_IOS_TR("Failed to create private key (invalid length or unsupported platform)")
                         : MAA_IOS_TR("Failed to create public key (invalid length or unsupported platform)");
    }
    return p;
}

/// ChaCha20-Poly1305（12 字节 nonce，16 字节标签附在密文尾部）。
///
/// 两个方向最容易写反的地方都在这里：解密时喂给 CipherUpdate 的必须**不含标签**，
/// 而 SET_TAG 要在 CipherFinal 之前——标签正是在 final 那一步校验的，所以
/// "密钥对不上"和"帧错位"都表现为 final 失败，错误文案要分得开。
bool chacha(bool encrypt, std::string_view key, std::string_view nonce, const Bytes& in, Bytes& out, std::string& err)
{
    static constexpr size_t kTagLen = 16;
    if (key.size() != 32 || nonce.size() != 12) {
        err = MAA_IOS_TR("ChaCha20-Poly1305 requires a 32-byte key and 12-byte nonce");
        return false;
    }
    if (!encrypt && in.size() < kTagLen) {
        err = MAA_IOS_TR("Ciphertext shorter than Poly1305 tag");
        return false;
    }
    CtxUp ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        err = MAA_IOS_TR("Failed to allocate EVP_CIPHER_CTX");
        return false;
    }
    if (EVP_CipherInit_ex(ctx.get(), EVP_chacha20_poly1305(), nullptr, nullptr, nullptr, encrypt ? 1 : 0) != 1
        || EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) != 1
        || EVP_CipherInit_ex(ctx.get(), nullptr, nullptr, u8(key), u8(nonce), encrypt ? 1 : 0) != 1) {
        err = MAA_IOS_TR("ChaCha20-Poly1305 initialization failed; check OpenSSL cipher support");
        return false;
    }
    const size_t body = encrypt ? in.size() : in.size() - kTagLen;
    out.assign(body + kTagLen, 0);
    int produced = 0;
    if (body > 0 && EVP_CipherUpdate(ctx.get(), out.data(), &produced, in.data(), static_cast<int>(body)) != 1) {
        err = MAA_IOS_TR("ChaCha20-Poly1305 update failed");
        return false;
    }
    int tail = 0;
    if (!encrypt) {
        if (EVP_CIPHER_CTX_ctrl(
                ctx.get(),
                EVP_CTRL_AEAD_SET_TAG,
                static_cast<int>(kTagLen),
                const_cast<unsigned char*>(in.data() + body))
            != 1) { // NOLINT
            err = MAA_IOS_TR("Failed to set Poly1305 tag");
            return false;
        }
        if (EVP_CipherFinal_ex(ctx.get(), out.data() + produced, &tail) != 1) {
            err = MAA_IOS_TR("Poly1305 authentication failed; check key derivation and frame boundaries");
            return false;
        }
        out.resize(static_cast<size_t>(produced) + tail);
        return true;
    }
    if (EVP_CipherFinal_ex(ctx.get(), out.data() + produced, &tail) != 1) {
        err = MAA_IOS_TR("ChaCha20-Poly1305 finalization failed");
        return false;
    }
    produced += tail;
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_GET_TAG, static_cast<int>(kTagLen), out.data() + produced) != 1) {
        err = MAA_IOS_TR("Failed to get Poly1305 tag");
        return false;
    }
    out.resize(static_cast<size_t>(produced) + kTagLen);
    return true;
}

} // namespace

std::optional<X25519KeyPair> x25519_keypair(std::string& err)
{
    PctxUp pctx(EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr));
    if (!pctx || EVP_PKEY_keygen_init(pctx.get()) <= 0) {
        err = MAA_IOS_TR("Failed to initialize X25519 key generation");
        return std::nullopt;
    }
    EVP_PKEY* raw = nullptr;
    if (EVP_PKEY_keygen(pctx.get(), &raw) <= 0) {
        err = MAA_IOS_TR("Failed to generate X25519 key");
        return std::nullopt;
    }
    PkeyUp pkey(raw);
    X25519KeyPair out;
    size_t len = out.pub.size();
    if (EVP_PKEY_get_raw_public_key(pkey.get(), out.pub.data(), &len) != 1 || len != out.pub.size()) {
        err = MAA_IOS_TR("Failed to get X25519 public key");
        return std::nullopt;
    }
    len = out.priv.size();
    if (EVP_PKEY_get_raw_private_key(pkey.get(), out.priv.data(), &len) != 1 || len != out.priv.size()) {
        err = MAA_IOS_TR("Failed to get X25519 private key");
        return std::nullopt;
    }
    return out;
}

std::optional<Bytes> random_bytes(size_t n, std::string& err)
{
    Bytes out(n);
    if (n > 0 && RAND_bytes(out.data(), static_cast<int>(n)) != 1) {
        err = MAA_IOS_TR("Failed to obtain cryptographic random bytes");
        return std::nullopt;
    }
    return out;
}

std::optional<Ed25519KeyPair> ed25519_keypair(std::string& err)
{
    const std::optional<Bytes> seed = random_bytes(32, err);
    if (!seed) {
        return std::nullopt;
    }
    const std::string_view seed_view(sv(*seed));
    PkeyUp key = raw_key(EVP_PKEY_ED25519, seed_view, true, err);
    if (!key) {
        return std::nullopt;
    }
    Ed25519KeyPair out;
    std::memcpy(out.seed.data(), seed->data(), out.seed.size());
    size_t len = out.pub.size();
    if (EVP_PKEY_get_raw_public_key(key.get(), out.pub.data(), &len) != 1 || len != out.pub.size()) {
        err = MAA_IOS_TR("Failed to get Ed25519 public key");
        return std::nullopt;
    }
    return out;
}

std::optional<Bytes> x25519_shared(const std::array<uint8_t, 32>& priv, std::string_view peer_pub, std::string& err)
{
    if (peer_pub.size() != 32) {
        err = MAA_IOS_TR("Peer X25519 public key must be 32 bytes");
        return std::nullopt;
    }
    const std::string_view priv_view(
        reinterpret_cast<const char*>(priv.data()), // NOLINT
        priv.size());
    PkeyUp ours = raw_key(EVP_PKEY_X25519, priv_view, true, err);
    if (!ours) {
        return std::nullopt;
    }
    PkeyUp theirs = raw_key(EVP_PKEY_X25519, peer_pub, false, err);
    if (!theirs) {
        return std::nullopt;
    }
    PctxUp dctx(EVP_PKEY_CTX_new(ours.get(), nullptr));
    size_t len = 0;
    if (!dctx || EVP_PKEY_derive_init(dctx.get()) <= 0 || EVP_PKEY_derive_set_peer(dctx.get(), theirs.get()) <= 0
        || EVP_PKEY_derive(dctx.get(), nullptr, &len) <= 0 || len != 32) {
        err = MAA_IOS_TR("X25519 shared secret computation failed");
        return std::nullopt;
    }
    Bytes out(len);
    if (EVP_PKEY_derive(dctx.get(), out.data(), &len) <= 0) {
        err = MAA_IOS_TR("X25519 shared secret computation failed");
        return std::nullopt;
    }
    // RFC 7748 §6.1：全零意味着对端是低阶点。不挡掉的话，后面所有密钥都从一个
    // 攻击者可预测的值派生——这就是"看着握上了手"的空会话。
    if (std::all_of(out.begin(), out.end(), [](uint8_t b) { return b == 0; })) {
        err = MAA_IOS_TR("X25519 shared secret is all zero (low-order peer public key)");
        return std::nullopt;
    }
    return out;
}

std::optional<Bytes> ed25519_sign(std::string_view seed, const Bytes& msg, std::string& err)
{
    if (seed.size() != 32) {
        err = MAA_IOS_TR("Ed25519 private key seed must be 32 bytes");
        return std::nullopt;
    }
    PkeyUp key = raw_key(EVP_PKEY_ED25519, seed, true, err);
    if (!key) {
        return std::nullopt;
    }
    MdCtxUp mdctx(EVP_MD_CTX_new());
    if (!mdctx || EVP_DigestSignInit(mdctx.get(), nullptr, nullptr, nullptr, key.get()) <= 0) {
        err = MAA_IOS_TR("Failed to initialize Ed25519 signing");
        return std::nullopt;
    }
    size_t len = 0;
    if (EVP_DigestSign(mdctx.get(), nullptr, &len, msg.data(), msg.size()) <= 0 || len != 64) {
        err = MAA_IOS_TR("Failed to get Ed25519 signature length");
        return std::nullopt;
    }
    Bytes out(len);
    if (EVP_DigestSign(mdctx.get(), out.data(), &len, msg.data(), msg.size()) <= 0) {
        err = MAA_IOS_TR("Ed25519 signing failed");
        return std::nullopt;
    }
    return out;
}

bool ed25519_verify(std::string_view public_key, const Bytes& msg, const Bytes& signature, std::string& err)
{
    if (public_key.size() != 32) {
        err = MAA_IOS_TR("Ed25519 public key must be 32 bytes");
        return false;
    }
    if (signature.size() != 64) {
        err = MAA_IOS_TR("Ed25519 signature must be 64 bytes");
        return false;
    }
    PkeyUp key = raw_key(EVP_PKEY_ED25519, public_key, false, err);
    if (!key) {
        return false;
    }
    MdCtxUp mdctx(EVP_MD_CTX_new());
    if (!mdctx) {
        err = MAA_IOS_TR("Failed to allocate EVP_MD_CTX");
        return false;
    }
    // PureEd25519 需要完整消息；digest 参数必须为 nullptr，不能使用流式 verify。
    if (EVP_DigestVerifyInit(mdctx.get(), nullptr, nullptr, nullptr, key.get()) != 1) {
        err = MAA_IOS_TR("Failed to initialize Ed25519 verification");
        return false;
    }
    const int result = EVP_DigestVerify(mdctx.get(), signature.data(), signature.size(), msg.data(), msg.size());
    if (result != 1) {
        err = result == 0 ? MAA_IOS_TR("Ed25519 signature verification failed") : MAA_IOS_TR("OpenSSL Ed25519 verification error");
        return false;
    }
    err.clear();
    return true;
}

std::optional<Bytes> hkdf_sha512(const Bytes& ikm, std::string_view salt, std::string_view info, size_t out_len, std::string& err)
{
    // EVP_PKEY_HKDF 在 OpenSSL 1.1.1 已提供，不要求 EVP_KDF 的 OpenSSL 3 API。
    // 统一采用旧接口的 info 上限；当前配对标签远小于此限制。
    constexpr size_t kMaxOutput = 255 * 64;
    constexpr size_t kMaxInfo = 1024;
    if (out_len > kMaxOutput || info.size() > kMaxInfo || ikm.size() > static_cast<size_t>(std::numeric_limits<int>::max())
        || salt.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        err = MAA_IOS_TR("HKDF-SHA512 input or output exceeds supported limits");
        return std::nullopt;
    }
    if (out_len == 0) {
        err.clear();
        return Bytes { };
    }
    PctxUp ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr));
    const uint8_t empty = 0;
    const auto* key = ikm.empty() ? &empty : ikm.data();
    if (!ctx || EVP_PKEY_derive_init(ctx.get()) <= 0 || EVP_PKEY_CTX_hkdf_mode(ctx.get(), EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND) <= 0
        || EVP_PKEY_CTX_set_hkdf_md(ctx.get(), EVP_sha512()) <= 0
        || EVP_PKEY_CTX_set1_hkdf_key(ctx.get(), key, static_cast<int>(ikm.size())) <= 0
        || (!salt.empty() && EVP_PKEY_CTX_set1_hkdf_salt(ctx.get(), u8(salt), static_cast<int>(salt.size())) <= 0)
        || (!info.empty() && EVP_PKEY_CTX_add1_hkdf_info(ctx.get(), u8(info), static_cast<int>(info.size())) <= 0)) {
        err = MAA_IOS_TR("Failed to initialize HKDF-SHA512");
        return std::nullopt;
    }
    Bytes out(out_len);
    size_t written = out_len;
    if (EVP_PKEY_derive(ctx.get(), out.data(), &written) <= 0 || written != out_len) {
        err = MAA_IOS_TR("HKDF-SHA512 derivation failed");
        return std::nullopt;
    }
    err.clear();
    return out;
}

std::optional<Bytes> chacha_seal(std::string_view key, std::string_view nonce, const Bytes& plain, std::string& err)
{
    Bytes out;
    if (!chacha(true, key, nonce, plain, out, err)) {
        return std::nullopt;
    }
    return out;
}

std::optional<Bytes> chacha_open(std::string_view key, std::string_view nonce, const Bytes& sealed, std::string& err)
{
    Bytes out;
    if (!chacha(false, key, nonce, sealed, out, err)) {
        return std::nullopt;
    }
    return out;
}

std::string b64_encode(std::string_view data)
{
    return util::base64_encode(data);
}

std::string b64_encode(const Bytes& data)
{
    return util::base64_encode(data);
}

std::optional<Bytes> b64_decode(std::string_view text, std::string& err)
{
    return util::base64_decode(text, err);
}

} // namespace maa::ios::wifi
