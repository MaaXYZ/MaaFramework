#include "wifi/Srp.h"
#include "i18n/Translation.h"

#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace maa::ios::wifi
{
namespace
{

/// RFC 5054 的 3072 位模数，与参考实现用的同一份常量。
constexpr const char* kPrimeHex =
    "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74"
    "020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F1437"
    "4FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
    "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF05"
    "98DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB"
    "9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3B"
    "E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF695581718"
    "3995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D04507A33"
    "A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7DB3970F85A6E1E4C7"
    "ABF5AE8CDB0933D71E8C94E04A25619DCEE3D2261AD2EE6BF12FFA06D98A0864"
    "D87602733EC86A64521F2B18177B200CBBE117577A615D6C770988C0BAD946E2"
    "08E24FA074E5AB3143DB5BFCE0FD108E4B82D120A93AD2CAFFFFFFFFFFFFFFFF";
constexpr int kGenerator = 5;
constexpr int kWidth = 384;
constexpr size_t kHashSize = 64;

void require(bool success, const char* operation)
{
    if (!success) {
        throw std::runtime_error(operation);
    }
}

struct Bn
{
    BIGNUM* v = nullptr;

    Bn()
    {
        v = BN_new();
        require(v != nullptr, "BN_new");
    }

    ~Bn() { BN_clear_free(v); }

    Bn(const Bn&) = delete;
    Bn& operator=(const Bn&) = delete;
};

Bytes sha512(const std::vector<Bytes>& parts)
{
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    require(ctx != nullptr, "EVP_MD_CTX_new");
    require(EVP_DigestInit_ex(ctx.get(), EVP_sha512(), nullptr) == 1, "EVP_DigestInit_ex");
    for (const auto& p : parts) {
        require(EVP_DigestUpdate(ctx.get(), p.data(), p.size()) == 1, "EVP_DigestUpdate");
    }
    Bytes out(kHashSize);
    unsigned int written = 0;
    require(EVP_DigestFinal_ex(ctx.get(), out.data(), &written) == 1 && written == kHashSize, "EVP_DigestFinal_ex");
    return out;
}

/// Apple 配对中用于哈希的最小大端整数编码，不添加符号字节或定长填充。
Bytes bytes_of_bn(const BIGNUM* n)
{
    const int len = BN_num_bytes(n);
    Bytes out(static_cast<size_t>(len));
    require(BN_bn2bin(n, out.data()) == len, "BN_bn2bin");
    return out;
}

/// 384 字节定长大端（参考实现的 pad()）。
Bytes pad_bn(const BIGNUM* n, int width)
{
    Bytes out(static_cast<size_t>(width), 0);
    require(BN_bn2binpad(n, out.data(), width) == width, "BN_bn2binpad");
    return out;
}

void bn_from_hex(const std::string& hex, Bn& dst)
{
    require(BN_hex2bn(&dst.v, hex.c_str()) == static_cast<int>(hex.size()), "BN_hex2bn");
}

Bytes sha512_int(const BIGNUM* n)
{
    return sha512({ bytes_of_bn(n) });
}

/// 将哈希按整数异或，再取最小大端编码。去掉前导零，零值保留一个字节。
Bytes xor_hashes(const Bytes& a, const Bytes& b)
{
    Bytes r(std::max(a.size(), b.size()), 0);
    for (size_t i = 0; i < a.size(); ++i) {
        r[r.size() - a.size() + i] ^= a[i];
    }
    for (size_t i = 0; i < b.size(); ++i) {
        r[r.size() - b.size() + i] ^= b[i];
    }
    size_t lead = 0;
    while (lead + 1 < r.size() && r[lead] == 0) {
        ++lead;
    }
    r.erase(r.begin(), r.begin() + static_cast<long>(lead));
    return r;
}

void clear_bytes(Bytes& bytes)
{
    if (!bytes.empty()) {
        OPENSSL_cleanse(bytes.data(), bytes.size());
    }
    bytes.clear();
}

} // namespace

SrpClient::SrpClient(std::string user, std::string password, std::string private_hex)
    : user_(std::move(user))
    , password_(std::move(password))
    , private_hex_(std::move(private_hex))
{
}

bool SrpClient::process(const Bytes& salt, const Bytes& server_public, std::string& err)
{
    // 每次计算独立发布结果。失败后不暴露上一轮或本轮未完成的密钥与证明。
    a_public_.clear();
    k_.clear();
    m1_.clear();
    m2_.clear();
    err.clear();
    if (server_public.empty() || server_public.size() > kWidth) {
        err = MAA_IOS_TR("SRP server public key is empty or exceeds 384 bytes");
        return false;
    }
    if (!private_hex_.empty()
        && (private_hex_.size() > kWidth * 2 || private_hex_.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)) {
        err = MAA_IOS_TR("SRP private key must be a nonzero hexadecimal integer of at most 3072 bits");
        return false;
    }
    try {
        std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)> ctx(BN_CTX_new(), BN_CTX_free);
        require(ctx != nullptr, "BN_CTX_new");
        Bn N;
        bn_from_hex(kPrimeHex, N);
        Bn g;
        require(BN_set_word(g.v, kGenerator) == 1, "BN_set_word");

        // k = H(N | PAD(g))
        const Bytes k = sha512({ bytes_of_bn(N.v), pad_bn(g.v, kWidth) });
        Bn kb;
        require(BN_bin2bn(k.data(), static_cast<int>(k.size()), kb.v) != nullptr, "BN_bin2bn(k)");

        // 测试可注入固定私钥；正常配对使用 OpenSSL 生成随机 1024 位指数。
        Bn a;
        if (!private_hex_.empty()) {
            bn_from_hex(private_hex_, a);
        }
        else {
            require(BN_rand(a.v, 1024, -1, 0) == 1, "BN_rand");
        }
        if (BN_is_zero(a.v)) {
            err = MAA_IOS_TR("SRP private key must be a nonzero hexadecimal integer of at most 3072 bits");
            return false;
        }
        BN_set_flags(a.v, BN_FLG_CONSTTIME);
        Bn A;
        require(BN_mod_exp(A.v, g.v, a.v, N.v, ctx.get()) == 1, "BN_mod_exp(A)");

        Bn B;
        require(BN_bin2bn(server_public.data(), static_cast<int>(server_public.size()), B.v) != nullptr, "BN_bin2bn(B)");
        Bn mod;
        require(BN_mod(mod.v, B.v, N.v, ctx.get()) == 1, "BN_mod(B)");
        if (BN_is_zero(mod.v)) {
            err = MAA_IOS_TR("SRP B is a multiple of N; rejected");
            return false;
        }

        // x = H(s | H(I ":" P))
        const Bytes inner = sha512({ bytes_of(user_), Bytes { ':' }, bytes_of(password_) });
        const Bytes x = sha512({ salt, inner });
        Bn xb;
        require(BN_bin2bn(x.data(), static_cast<int>(x.size()), xb.v) != nullptr, "BN_bin2bn(x)");
        BN_set_flags(xb.v, BN_FLG_CONSTTIME);

        // u = H(PAD(A) | PAD(B))
        const Bytes u = sha512({ pad_bn(A.v, kWidth), pad_bn(B.v, kWidth) });
        Bn ub;
        require(BN_bin2bn(u.data(), static_cast<int>(u.size()), ub.v) != nullptr, "BN_bin2bn(u)");
        require(!BN_is_zero(ub.v), "SRP u == 0");

        // v = g^x mod N; S = (B - k*v)^(a + u*x) mod N
        Bn v, kv, base, ux, exponent, S;
        require(BN_mod_exp(v.v, g.v, xb.v, N.v, ctx.get()) == 1, "BN_mod_exp(v)");
        require(BN_mod_mul(kv.v, kb.v, v.v, N.v, ctx.get()) == 1, "BN_mod_mul");
        require(BN_mod_sub(base.v, B.v, kv.v, N.v, ctx.get()) == 1, "BN_mod_sub");
        require(BN_mul(ux.v, ub.v, xb.v, ctx.get()) == 1, "BN_mul");
        require(BN_add(exponent.v, a.v, ux.v) == 1, "BN_add");
        BN_set_flags(exponent.v, BN_FLG_CONSTTIME);
        require(BN_mod_exp(S.v, base.v, exponent.v, N.v, ctx.get()) == 1, "BN_mod_exp(S)");

        // Apple 适配使用最小大端编码：K = H(S)，M1 / M2 不对 A、B 做 PAD。
        auto public_key = bytes_of_bn(A.v);
        auto key = sha512({ bytes_of_bn(S.v) });
        const Bytes hxor = xor_hashes(sha512_int(N.v), sha512_int(g.v));
        const Bytes huser = sha512({ bytes_of(user_) });
        auto proof = sha512({ hxor, huser, salt, public_key, bytes_of_bn(B.v), key });
        auto server_proof = sha512({ public_key, proof, key });
        a_public_ = std::move(public_key);
        k_ = std::move(key);
        m1_ = std::move(proof);
        m2_ = std::move(server_proof);
        return true;
    }
    catch (const std::runtime_error& failure) {
        err = MAA_IOS_TR("SRP cryptographic operation failed: ") + std::string(failure.what());
        return false;
    }
}

bool SrpClient::verify_server_proof(const Bytes& m2) const
{
    return m2_.size() == kHashSize && m2.size() == kHashSize && CRYPTO_memcmp(m2.data(), m2_.data(), kHashSize) == 0;
}

SrpServer::SrpServer(std::string user, std::string password, std::string private_hex)
    : user_(std::move(user))
    , password_(std::move(password))
    , private_hex_(std::move(private_hex))
{
}

SrpServer::~SrpServer()
{
    clear_secrets();
    clear_bytes(k_);
    clear_bytes(m2_);
    if (!password_.empty()) {
        OPENSSL_cleanse(password_.data(), password_.size());
    }
    if (!private_hex_.empty()) {
        OPENSSL_cleanse(private_hex_.data(), private_hex_.size());
    }
}

void SrpServer::clear_secrets()
{
    clear_bytes(b_private_);
    clear_bytes(verifier_);
    awaiting_client_ = false;
}

bool SrpServer::initialize(const Bytes& salt, std::string& err)
{
    clear_secrets();
    clear_bytes(k_);
    clear_bytes(m2_);
    salt_.clear();
    b_public_.clear();
    err.clear();
    if (salt.empty() || salt.size() > 255) {
        err = MAA_IOS_TR("SRP salt must contain 1 to 255 bytes");
        return false;
    }
    if (!private_hex_.empty()
        && (private_hex_.size() > kWidth * 2 || private_hex_.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)) {
        err = MAA_IOS_TR("SRP private key must be a nonzero hexadecimal integer of at most 3072 bits");
        return false;
    }
    try {
        std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)> ctx(BN_CTX_new(), BN_CTX_free);
        require(ctx != nullptr, "BN_CTX_new");
        Bn N, g, b, x, v, kb, gb, kv, B;
        bn_from_hex(kPrimeHex, N);
        require(BN_set_word(g.v, kGenerator) == 1, "BN_set_word");
        if (!private_hex_.empty()) {
            bn_from_hex(private_hex_, b);
        }
        else {
            require(BN_priv_rand(b.v, 1024, BN_RAND_TOP_ONE, BN_RAND_BOTTOM_ANY) == 1, "BN_priv_rand");
        }
        if (BN_is_zero(b.v)) {
            err = MAA_IOS_TR("SRP private key must be a nonzero hexadecimal integer of at most 3072 bits");
            return false;
        }
        BN_set_flags(b.v, BN_FLG_CONSTTIME);
        const auto inner = sha512({ bytes_of(user_), Bytes { ':' }, bytes_of(password_) });
        const auto x_hash = sha512({ salt, inner });
        require(BN_bin2bn(x_hash.data(), static_cast<int>(x_hash.size()), x.v) != nullptr, "BN_bin2bn(x)");
        BN_set_flags(x.v, BN_FLG_CONSTTIME);
        require(BN_mod_exp(v.v, g.v, x.v, N.v, ctx.get()) == 1, "BN_mod_exp(v)");
        const auto multiplier = sha512({ bytes_of_bn(N.v), pad_bn(g.v, kWidth) });
        require(BN_bin2bn(multiplier.data(), static_cast<int>(multiplier.size()), kb.v) != nullptr, "BN_bin2bn(k)");
        // RFC 5054 §2.5.3: B = (k*v + g^b) mod N.
        require(BN_mod_exp(gb.v, g.v, b.v, N.v, ctx.get()) == 1, "BN_mod_exp(g^b)");
        require(BN_mod_mul(kv.v, kb.v, v.v, N.v, ctx.get()) == 1, "BN_mod_mul(k*v)");
        require(BN_mod_add(B.v, kv.v, gb.v, N.v, ctx.get()) == 1, "BN_mod_add(B)");
        require(!BN_is_zero(B.v), "SRP B == 0");
        // 所有计算成功后再发布 challenge，密钥和证明仍为空。
        auto private_value = bytes_of_bn(b.v);
        auto verifier = bytes_of_bn(v.v);
        auto public_value = bytes_of_bn(B.v);
        salt_ = salt;
        b_private_ = std::move(private_value);
        verifier_ = std::move(verifier);
        b_public_ = std::move(public_value);
        awaiting_client_ = true;
        return true;
    }
    catch (const std::runtime_error& failure) {
        clear_secrets();
        err = MAA_IOS_TR("SRP cryptographic operation failed: ") + std::string(failure.what());
        return false;
    }
}

bool SrpServer::process(const Bytes& client_public, const Bytes& client_proof, std::string& err)
{
    clear_bytes(k_);
    clear_bytes(m2_);
    err.clear();
    if (!awaiting_client_) {
        clear_secrets();
        err = MAA_IOS_TR("SRP server requires a new initialized challenge");
        return false;
    }
    // 即使输入不合法也消耗本次 challenge，不能对同一 b 猜测多个 PIN/证明。
    awaiting_client_ = false;
    if (client_public.empty() || client_public.size() > kWidth) {
        clear_secrets();
        err = MAA_IOS_TR("SRP client public key is empty or exceeds 384 bytes");
        return false;
    }
    if (client_proof.size() != kHashSize) {
        clear_secrets();
        err = MAA_IOS_TR("SRP client proof must contain 64 bytes");
        return false;
    }
    try {
        std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)> ctx(BN_CTX_new(), BN_CTX_free);
        require(ctx != nullptr, "BN_CTX_new");
        Bn N, g, A, b, v, mod, u, vu, base, S;
        bn_from_hex(kPrimeHex, N);
        require(BN_set_word(g.v, kGenerator) == 1, "BN_set_word");
        require(BN_bin2bn(client_public.data(), static_cast<int>(client_public.size()), A.v) != nullptr, "BN_bin2bn(A)");
        require(BN_mod(mod.v, A.v, N.v, ctx.get()) == 1, "BN_mod(A)");
        if (BN_is_zero(mod.v)) {
            clear_secrets();
            err = MAA_IOS_TR("SRP A is a multiple of N; rejected");
            return false;
        }
        Bn B;
        require(BN_bin2bn(b_public_.data(), static_cast<int>(b_public_.size()), B.v) != nullptr, "BN_bin2bn(B)");
        const auto u_hash = sha512({ pad_bn(A.v, kWidth), pad_bn(B.v, kWidth) });
        require(BN_bin2bn(u_hash.data(), static_cast<int>(u_hash.size()), u.v) != nullptr, "BN_bin2bn(u)");
        require(!BN_is_zero(u.v), "SRP u == 0");
        require(BN_bin2bn(b_private_.data(), static_cast<int>(b_private_.size()), b.v) != nullptr, "BN_bin2bn(b)");
        require(BN_bin2bn(verifier_.data(), static_cast<int>(verifier_.size()), v.v) != nullptr, "BN_bin2bn(v)");
        BN_set_flags(b.v, BN_FLG_CONSTTIME);
        // RFC 5054 §2.6: S = (A * v^u)^b mod N.
        require(BN_mod_exp(vu.v, v.v, u.v, N.v, ctx.get()) == 1, "BN_mod_exp(v^u)");
        require(BN_mod_mul(base.v, A.v, vu.v, N.v, ctx.get()) == 1, "BN_mod_mul(A*v^u)");
        require(BN_mod_exp(S.v, base.v, b.v, N.v, ctx.get()) == 1, "BN_mod_exp(S)");
        auto key = sha512({ bytes_of_bn(S.v) });
        const auto a_public = bytes_of_bn(A.v);
        const auto hxor = xor_hashes(sha512_int(N.v), sha512_int(g.v));
        const auto expected = sha512({ hxor, sha512({ bytes_of(user_) }), salt_, a_public, b_public_, key });
        if (CRYPTO_memcmp(client_proof.data(), expected.data(), kHashSize) != 0) {
            clear_bytes(key);
            clear_secrets();
            err = MAA_IOS_TR("SRP client proof mismatch; pairing aborted");
            return false;
        }
        auto proof = sha512({ a_public, expected, key });
        clear_secrets();
        k_ = std::move(key);
        m2_ = std::move(proof);
        return true;
    }
    catch (const std::runtime_error& failure) {
        clear_secrets();
        err = MAA_IOS_TR("SRP cryptographic operation failed: ") + std::string(failure.what());
        return false;
    }
}

} // namespace maa::ios::wifi
