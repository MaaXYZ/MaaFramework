#include "util/Base64.h"
#include <limits>
#include <openssl/evp.h>
#include <stdexcept>

namespace maa::ios::util
{
std::string base64_encode(std::string_view data)
{
    if (data.empty()) {
        return { };
    }
    const auto max = static_cast<size_t>(std::numeric_limits<int>::max());
    if (data.size() > (max / 4) * 3) {
        throw std::length_error("Base64 input too large");
    }
    std::string out(4 * ((data.size() + 2) / 3) + 1, '\0');
    const int n = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(out.data()),
        reinterpret_cast<const unsigned char*>(data.data()),
        static_cast<int>(data.size()));
    out.resize(static_cast<size_t>(n));
    return out;
}

std::string base64_encode(std::span<const uint8_t> data)
{
    if (data.empty()) {
        return { };
    }
    return base64_encode(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
}

std::optional<std::vector<uint8_t>> base64_decode(std::string_view text, std::string& err)
{
    err.clear();
    std::string clean;
    clean.reserve(text.size());
    for (char c : text) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            clean += c;
        }
    }
    if (clean.empty()) {
        return std::vector<uint8_t> { };
    }
    const auto pad = clean.find('=');
    const auto body = pad == std::string::npos ? clean.size() : pad;
    if (clean.substr(0, body).find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/") != std::string::npos
        || (pad != std::string::npos
            && (clean.size() % 4 != 0 || clean.size() - pad > 2 || clean.find_first_not_of('=', pad) != std::string::npos))
        || body % 4 == 1) {
        err = "Invalid base64 character or padding";
        return std::nullopt;
    }
    if (pad == std::string::npos) {
        clean.append((4 - clean.size() % 4) % 4, '=');
    }
    if (clean.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        err = "Base64 input too large";
        return std::nullopt;
    }
    const size_t padding = clean.ends_with("==") ? 2 : (clean.ends_with('=') ? 1 : 0);
    std::vector<uint8_t> out(clean.size() / 4 * 3);
    const int n = EVP_DecodeBlock(out.data(), reinterpret_cast<const unsigned char*>(clean.data()), static_cast<int>(clean.size()));
    if (n < 0 || static_cast<size_t>(n) < padding) {
        err = "Base64 decode failed";
        return std::nullopt;
    }
    out.resize(static_cast<size_t>(n) - padding);
    return out;
}
} // namespace maa::ios::util
