#include "wifi/DiscoveryIdentity.h"


#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/core_names.h>
#include <openssl/params.h>
#endif

#include <algorithm>
#include <memory>

namespace maa::ios::wifi
{

std::optional<std::array<uint8_t, 8>> siphash24(std::string_view key, std::string_view message, std::string& err)
{
    err.clear();
    if (key.size() != 16) {
        err = "SipHash requires a 16-byte key";
        return std::nullopt;
    }

    std::array<uint8_t, 8> output { };
    size_t output_size = output.size();
    bool success = false;
    const auto* key_bytes = reinterpret_cast<const unsigned char*>(key.data());
    const auto* message_bytes = reinterpret_cast<const unsigned char*>(message.data());

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    std::unique_ptr<EVP_MAC, decltype(&EVP_MAC_free)> mac(EVP_MAC_fetch(nullptr, "SIPHASH", nullptr), EVP_MAC_free);
    std::unique_ptr<EVP_MAC_CTX, decltype(&EVP_MAC_CTX_free)> ctx(mac ? EVP_MAC_CTX_new(mac.get()) : nullptr, EVP_MAC_CTX_free);
    unsigned int compression_rounds = 2;
    unsigned int final_rounds = 4;
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_size_t(OSSL_MAC_PARAM_SIZE, &output_size),
        OSSL_PARAM_construct_uint(OSSL_MAC_PARAM_C_ROUNDS, &compression_rounds),
        OSSL_PARAM_construct_uint(OSSL_MAC_PARAM_D_ROUNDS, &final_rounds),
        OSSL_PARAM_construct_end(),
    };
    // OpenSSL 默认可能选择 128 位 SipHash；该变体不能截断成 Apple 所需的结果。
    success = ctx && EVP_MAC_init(ctx.get(), key_bytes, key.size(), params) == 1
              && EVP_MAC_update(ctx.get(), message_bytes, message.size()) == 1
              && EVP_MAC_final(ctx.get(), output.data(), &output_size, output.size()) == 1;
#else
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkey(
        EVP_PKEY_new_raw_private_key(EVP_PKEY_SIPHASH, nullptr, key_bytes, key.size()),
        EVP_PKEY_free);
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    EVP_PKEY_CTX* pkey_ctx = nullptr;
    // OpenSSL 1.1.1 的 SipHash 实现固定为 2/4 轮；使用 EVP 控制设置 8 字节输出。
    success = pkey && ctx && EVP_DigestSignInit(ctx.get(), &pkey_ctx, nullptr, nullptr, pkey.get()) == 1
              && EVP_PKEY_CTX_ctrl(
                     pkey_ctx,
                     EVP_PKEY_SIPHASH,
                     EVP_PKEY_OP_SIGNCTX,
                     EVP_PKEY_CTRL_SET_DIGEST_SIZE,
                     static_cast<int>(output.size()),
                     nullptr)
                     == 1
              && EVP_DigestSignUpdate(ctx.get(), message_bytes, message.size()) == 1
              && EVP_DigestSignFinal(ctx.get(), output.data(), &output_size) == 1;
#endif

    if (!success || output_size != output.size()) {
        err = "OpenSSL SipHash computation failed";
        return std::nullopt;
    }
    return output;
}

DiscoveryIdentityMatch match_advertisement(std::string_view identifier, std::string_view auth_tag, const std::vector<PairRecord>& records)
{
    DiscoveryIdentityMatch result;
    if (identifier.empty() || identifier.size() > 255
        || std::any_of(identifier.begin(), identifier.end(), [](unsigned char c) { return c <= 0x20 || c == 0x7f; })) {
        result.status = DiscoveryIdentityStatus::invalid_advertisement;
        result.error = "Invalid mDNS identifier";
        return result;
    }

    std::array<uint8_t, 6> tag { };
    if (!auth_tag.empty()) {
        static constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        if (auth_tag.size() != 8 || auth_tag.find_first_not_of(alphabet) != std::string_view::npos
            || EVP_DecodeBlock(tag.data(), reinterpret_cast<const unsigned char*>(auth_tag.data()), static_cast<int>(auth_tag.size()))
                   != static_cast<int>(tag.size())) {
            result.status = DiscoveryIdentityStatus::invalid_advertisement;
            result.error = "Invalid mDNS authTag (expected 8 Base64 characters)";
            return result;
        }
    }

    for (size_t i = 0; i < records.size(); ++i) {
        const auto& record = records[i];
        if (auth_tag.empty() || record.peer_alt_irk.size() != 16) {
            continue;
        }
        const auto hash = siphash24(sv(record.peer_alt_irk), identifier, result.error);
        if (!hash) {
            result.status = DiscoveryIdentityStatus::crypto_error;
            result.auth_tag_matches.clear();
            return result;
        }
        std::array<uint8_t, 6> expected { };
        std::reverse_copy(hash->begin(), hash->begin() + expected.size(), expected.begin());
        if (CRYPTO_memcmp(expected.data(), tag.data(), tag.size()) == 0) {
            result.auth_tag_matches.push_back(i);
        }
    }

    if (result.auth_tag_matches.size() == 1) {
        result.status = DiscoveryIdentityStatus::matched;
        result.record_index = result.auth_tag_matches.front();
    }
    else if (result.auth_tag_matches.size() > 1) {
        result.status = DiscoveryIdentityStatus::ambiguous;
    }
    return result;
}

} // namespace maa::ios::wifi
