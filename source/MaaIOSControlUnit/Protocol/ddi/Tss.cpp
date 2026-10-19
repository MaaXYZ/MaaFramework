#include "ddi/Tss.h"
#include "ddi/AppleTssTrust.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <curl/curl.h>
#include <exception>
#include <limits>
#include <memory>
#include <openssl/rand.h>

namespace maa::ios::ddi
{
namespace
{
using V = plist::Value;
constexpr size_t kMaxResponse = 4u << 20;
constexpr size_t kMaxTicket = 1u << 20;
constexpr size_t kMaxRequest = 2u << 20;
constexpr char kTssUrl[] = "https://gs.apple.com/TSS/controller?action=2";

bool valid_shape(const V& value, unsigned depth = 0)
{
    if (depth > 64 || value.keys.size() != value.values.size()) {
        return false;
    }
    for (const auto& child : value.values) {
        if (!valid_shape(child, depth + 1)) {
            return false;
        }
    }
    for (const auto& child : value.array) {
        if (!valid_shape(child, depth + 1)) {
            return false;
        }
    }
    return true;
}

std::optional<uint64_t> number(const V* value)
{
    if (!value) {
        return { };
    }
    if (value->is_uint()) {
        return value->unsigned_integer;
    }
    if (value->is_int() && value->integer >= 0) {
        return static_cast<uint64_t>(value->integer);
    }
    if (!value->is_string() || value->string.empty()) {
        return { };
    }
    std::string_view text = value->string;
    int base = 10;
    if (text.starts_with("0x") || text.starts_with("0X")) {
        base = 16;
        text.remove_prefix(2);
    }
    uint64_t out = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), out, base);
    if (ec != std::errc { } || end != text.data() + text.size()) {
        return { };
    }
    return out;
}

V integer(uint64_t number)
{
    return number <= static_cast<uint64_t>(INT64_MAX) ? V::Int(static_cast<int64_t>(number)) : V::UInt(number);
}

bool truth(const V* value)
{
    if (!value) {
        return false;
    }
    if (value->is_bool()) {
        return value->boolean;
    }
    auto n = number(value);
    return n && *n != 0;
}

std::optional<V> request_base(std::string& error)
{
    std::array<uint8_t, 16> bytes { };
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1) {
        error = "Cannot generate DDI signing request identifier";
        return std::nullopt;
    }
    bytes[6] = (bytes[6] & 15) | 0x40;
    bytes[8] = (bytes[8] & 63) | 0x80;
    constexpr char hex[] = "0123456789ABCDEF";
    std::string uuid;
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            uuid += '-';
        }
        uuid += hex[bytes[i] >> 4];
        uuid += hex[bytes[i] & 15];
    }
    auto request = V::Dict();
    request.set("@HostPlatformInfo", V::Str("mac"));
    request.set("@VersionInfo", V::Str("libauthinstall-1104.0.9"));
    request.set("@UUID", V::Str(std::move(uuid)));
    return request;
}

std::optional<V> without_info(const V& entry, std::string& error)
{
    if (!entry.is_dict() || entry.keys.size() != entry.values.size()) {
        error = "DDI signing manifest component is malformed";
        return std::nullopt;
    }
    auto result = V::Dict();
    for (size_t i = 0; i < entry.keys.size(); ++i) {
        if (entry.keys[i] != "Info") {
            result.set(entry.keys[i], entry.values[i]);
        }
    }
    return result;
}

bool apply_rules(V& entry, const V* rules, std::string& error)
{
    if (!rules) {
        return true;
    }
    if (!rules->is_array()) {
        error = "DDI restore request rules are malformed";
        return false;
    }
    for (const auto& rule : rules->array) {
        const auto *conditions = rule.find("Conditions"), *actions = rule.find("Actions");
        if (!(rule.is_dict() && conditions && conditions->is_dict() && actions && actions->is_dict()
              && conditions->keys.size() == conditions->values.size() && actions->keys.size() == actions->values.size())) {
            error = "DDI restore request rules are malformed";
            return false;
        }
        bool match = true;
        for (size_t i = 0; i < conditions->keys.size(); ++i) {
            const auto& key = conditions->keys[i];
            // Developer PDI uses production/security mode and Image4, never DFU or demotion.
            const bool known =
                key == "ApRawProductionMode" || key == "ApCurrentProductionMode" || key == "ApRawSecurityMode" || key == "ApRequiresImage4";
            const auto& value = conditions->values[i];
            const auto n = number(&value);
            if (!known || !((value.is_bool() && value.boolean) || (n && *n == 1))) {
                match = false;
                break;
            }
        }
        if (!match) {
            continue;
        }
        for (size_t i = 0; i < actions->keys.size(); ++i) {
            const auto& value = actions->values[i];
            const auto n = number(&value);
            if (n && *n == 255) {
                continue; // 255 means leave this property unchanged.
            }
            entry.set(actions->keys[i], value);
        }
    }
    return true;
}

struct CurlContext
{
    std::string response;
    const TssOptions* options;
    bool oversized = false;
    bool callback_failed = false;
    bool cancelled = false;
};

size_t receive(char* data, size_t size, size_t count, void* opaque) noexcept
{
    auto& context = *static_cast<CurlContext*>(opaque);
    if (size && count > SIZE_MAX / size) {
        context.oversized = true;
        return 0;
    }
    const auto bytes = size * count;
    if (bytes > kMaxResponse - context.response.size()) {
        context.oversized = true;
        return 0;
    }
    try {
        context.response.append(data, bytes);
        return bytes;
    }
    catch (...) {
        context.callback_failed = true;
        return 0;
    }
}

int progress(void* opaque, curl_off_t, curl_off_t, curl_off_t, curl_off_t) noexcept
{
    auto& context = *static_cast<CurlContext*>(opaque);
    try {
        context.cancelled = context.options->cancelled && context.options->cancelled();
        return context.cancelled ? 1 : 0;
    }
    catch (...) {
        context.callback_failed = true;
        return 1;
    }
}
} // namespace

std::optional<V> build_personalized_tss_request(
    const V& build_manifest,
    const V& identifiers,
    uint64_t ecid,
    const std::vector<uint8_t>& nonce,
    std::string& error)
{
    error.clear();
    try {
        if (!valid_shape(build_manifest) || !valid_shape(identifiers)) {
            error = "DDI signing metadata is malformed";
            return std::nullopt;
        }
        if (nonce.empty() || nonce.size() > 64) {
            error = "DDI signing nonce has an invalid length";
            return std::nullopt;
        }
        if (ecid == 0 || !identifiers.is_dict()) {
            error = "DDI signing device identifiers are missing";
            return std::nullopt;
        }
        const auto board = number(identifiers.find("BoardId")), chip = number(identifiers.find("ChipID"));
        if (!board || !chip) {
            error = "DDI signing metadata contains an invalid numeric identifier";
            return std::nullopt;
        }
        const auto* identities = build_manifest.find("BuildIdentities");
        if (!identities || !identities->is_array()) {
            error = "DDI signing build identities are missing";
            return std::nullopt;
        }
        const V* identity = nullptr;
        for (const auto& candidate : identities->array) {
            if (number(candidate.find("ApBoardID")) != board || number(candidate.find("ApChipID")) != chip) {
                continue;
            }
            const auto* info = candidate.find("Info");
            const auto* variant = info ? info->find("Variant") : nullptr;
            if (!variant || !variant->is_string() || !variant->string.ends_with("Developer PDI")) {
                continue;
            }
            if (identity) {
                error = "DDI signing hardware identity is ambiguous";
                return std::nullopt;
            }
            identity = &candidate;
        }
        if (!identity) {
            error = "DDI signing manifest has no matching hardware identity";
            return std::nullopt;
        }
        const auto* manifest = identity->find("Manifest");
        if (!manifest || !manifest->is_dict()) {
            error = "DDI signing component manifest is missing";
            return std::nullopt;
        }
        auto base = request_base(error);
        if (!base) {
            return std::nullopt;
        }
        auto request = std::move(*base);
        for (size_t i = 0; i < identifiers.keys.size(); ++i) {
            if (identifiers.keys[i].starts_with("Ap,")) {
                request.set(identifiers.keys[i], identifiers.values[i]);
            }
        }
        request.set("@ApImg4Ticket", V::Bool(true));
        request.set("@BBTicket", V::Bool(true));
        request.set("ApBoardID", integer(*board));
        request.set("ApChipID", integer(*chip));
        request.set("ApECID", integer(ecid));
        request.set("ApNonce", V::OfData(nonce));
        request.set("ApProductionMode", V::Bool(true));
        request.set("ApSecurityDomain", V::Int(1));
        request.set("ApSecurityMode", V::Bool(true));
        request.set("SepNonce", V::OfData(std::vector<uint8_t>(20, 0)));
        request.set("UID_MODE", V::Bool(false));
        const auto* trust = manifest->find("LoadableTrustCache");
        const auto* trust_info = trust ? trust->find("Info") : nullptr;
        const auto* common_rules = trust_info ? trust_info->find("RestoreRequestRules") : nullptr;
        bool has_dmg = false, has_trust = false;
        for (size_t i = 0; i < manifest->keys.size(); ++i) {
            const auto& entry = manifest->values[i];
            const auto* info = entry.find("Info");
            if (!info || !info->is_dict() || !truth(entry.find("Trusted"))) {
                continue;
            }
            auto signed_entry = without_info(entry, error);
            // PDI's shared trust-cache rules also describe the DMG, whose Info has no rules.
            if (!signed_entry || !apply_rules(*signed_entry, common_rules, error)) {
                return std::nullopt;
            }
            const auto* digest = entry.find("Digest");
            if (digest && digest->kind != plist::Kind::Data) {
                error = "DDI signing component digest is malformed";
                return std::nullopt;
            }
            if (!digest) {
                signed_entry->set("Digest", V::OfData({ }));
            }
            request.set(manifest->keys[i], std::move(*signed_entry));
            has_dmg |= manifest->keys[i] == "PersonalizedDMG";
            has_trust |= manifest->keys[i] == "LoadableTrustCache";
        }
        if (!has_dmg || !has_trust) {
            error = "DDI signing manifest lacks trusted image components";
            return std::nullopt;
        }
        return request;
    }
    catch (const std::exception& e) {
        error = e.what();
        return { };
    }
}

std::optional<V> build_cryptex_tss_request(const V& identity, const V& chip_instance, const std::vector<uint8_t>& nonce, std::string& error)
{
    error.clear();
    try {
        if (!valid_shape(identity) || !valid_shape(chip_instance)) {
            error = "DDI signing metadata is malformed";
            return std::nullopt;
        }
        if (nonce.empty() || nonce.size() > 64) {
            error = "DDI signing nonce has an invalid length";
            return std::nullopt;
        }
        if (!identity.is_dict() || !chip_instance.is_dict()) {
            error = "Cryptex signing identifiers are missing";
            return std::nullopt;
        }
        auto base = request_base(error);
        if (!base) {
            return std::nullopt;
        }
        auto request = std::move(*base);
        request.set("@Cryptex1,Ticket", V::Bool(true));
        for (const char* key : { "Cryptex1,ChipID", "Cryptex1,ProductClass" }) {
            const auto value = number(identity.find(key));
            if (!value) {
                error = "DDI signing metadata contains an invalid numeric identifier";
                return std::nullopt;
            }
            request.set(key, integer(*value));
        }
        for (const char* key : { "Cryptex1,Type",
                                 "Cryptex1,SubType",
                                 "Cryptex1,UseProductClass",
                                 "Cryptex1,NonceDomain",
                                 "Cryptex1,Version",
                                 "Cryptex1,PreauthorizationVersion" }) {
            const auto* value = identity.find(key);
            if (!value) {
                error = "Cryptex signing build identity is incomplete";
                return std::nullopt;
            }
            request.set(key, *value);
        }
        const auto chip = number(chip_instance.find("img4_chip_chip"));
        const auto ecid = number(chip_instance.find("img4_chip_ecid"));
        if (!chip || !ecid) {
            error = "DDI signing metadata contains an invalid numeric identifier";
            return std::nullopt;
        }
        if (*ecid == 0) {
            error = "Cryptex signing device ECID is missing";
            return std::nullopt;
        }
        const auto* production = chip_instance.find("img4_chip_cpro");
        if (!(production && (production->is_bool() || number(production)))) {
            error = "Cryptex signing production mode is missing";
            return std::nullopt;
        }
        std::vector<uint8_t> udid;
        for (const auto number : { *chip, *ecid }) {
            for (int shift = 56; shift >= 0; shift -= 8) {
                udid.push_back(static_cast<uint8_t>(number >> shift));
            }
        }
        request.set("Cryptex1,UDID", V::OfData(std::move(udid)));
        request.set("Cryptex1,ProductionMode", V::Bool(truth(production)));
        request.set("Cryptex1,Nonce", V::OfData(nonce));
        request.set("Cryptex1,UniqueTagList", V::OfData({ }));
        const auto* manifest = identity.find("Manifest");
        if (!manifest || !manifest->is_dict()) {
            error = "Cryptex signing component manifest is missing";
            return std::nullopt;
        }
        size_t components = 0;
        for (size_t i = 0; i < manifest->keys.size(); ++i) {
            if (!manifest->keys[i].starts_with("Cryptex1,")) {
                continue;
            }
            const auto& entry = manifest->values[i];
            const auto* info = entry.find("Info");
            if (!info || !truth(info->find("Personalize"))) {
                continue;
            }
            const auto* digest = entry.find("Digest");
            if (!(digest && digest->kind == plist::Kind::Data && !digest->data.empty() && digest->data.size() <= 64)) {
                error = "Cryptex signing component digest is missing";
                return std::nullopt;
            }
            auto signed_entry = V::Dict();
            signed_entry.set("Digest", *digest);
            request.set(manifest->keys[i], std::move(signed_entry));
            ++components;
        }
        if (components == 0) {
            error = "Cryptex signing manifest has no personalized components";
            return std::nullopt;
        }
        return request;
    }
    catch (const std::exception& e) {
        error = e.what();
        return { };
    }
}

namespace detail
{
bool configure_tss_https(CURL* curl, std::string& error)
{
    error.clear();
    try {
        if (!curl) {
            error = "Cannot create DDI signing HTTPS transport";
            return false;
        }
#define TLS_OPT(option, value)                                  \
    if (curl_easy_setopt(curl, option, value) != CURLE_OK) {    \
        error = "Cannot configure DDI signing HTTPS transport"; \
        return false;                                           \
    }
        TLS_OPT(CURLOPT_URL, kTssUrl);
#if LIBCURL_VERSION_NUM >= 0x075500
        TLS_OPT(CURLOPT_PROTOCOLS_STR, "https");
#else
        TLS_OPT(CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
#endif
        // A signing POST must never be forwarded to another origin.
        TLS_OPT(CURLOPT_FOLLOWLOCATION, 0L);
        TLS_OPT(CURLOPT_SSL_VERIFYPEER, 1L);
        TLS_OPT(CURLOPT_SSL_VERIFYHOST, 2L);
        TLS_OPT(CURLOPT_SSLVERSION, static_cast<long>(CURL_SSLVERSION_TLSv1_2));
#ifndef __APPLE__
#if LIBCURL_VERSION_NUM >= 0x074D00
        const auto* version = curl_version_info(CURLVERSION_NOW);
        if (!(version && version->version_num >= 0x074D00 && version->ssl_version)) {
            error = "DDI signing requires libcurl 7.77.0 or later with CA bundle support";
            return false;
        }
        const std::string_view backend = version->ssl_version;
        // These versions are documented by libcurl's CAINFO_BLOB API. Reject
        // unknown/older backends instead of falling back to unverified TLS.
        const bool supported = backend.starts_with("OpenSSL/") || backend.starts_with("LibreSSL/") || backend.starts_with("BoringSSL")
                               || backend.starts_with("AWS-LC") || backend.starts_with("Schannel")
                               || (backend.starts_with("GnuTLS/") && version->version_num >= 0x081200)
                               || (backend.starts_with("mbedTLS/") && version->version_num >= 0x075100)
                               || (backend.starts_with("rustls/") && version->version_num >= 0x075200)
                               || (backend.starts_with("wolfSSL/") && version->version_num >= 0x080200);
        if (!supported) {
            error = "This libcurl TLS backend cannot use the DDI signing CA bundle";
            return false;
        }
        // TSS uses one dedicated public Apple root, independent of incomplete
        // VM/system CA stores. Do not widen this handle with native/default CAs.
        TLS_OPT(CURLOPT_SSL_OPTIONS, 0L);
        TLS_OPT(CURLOPT_CAINFO, static_cast<const char*>(nullptr));
        const auto clear_path = curl_easy_setopt(curl, CURLOPT_CAPATH, static_cast<const char*>(nullptr));
        // Schannel/Rustls have no CA-directory support. NOT_BUILT_IN here
        // means there is no directory to clear, not a failed trust-bundle load.
        if (!(clear_path == CURLE_OK || clear_path == CURLE_NOT_BUILT_IN)) {
            error = "Cannot configure DDI signing HTTPS transport";
            return false;
        }
        curl_blob root { const_cast<char*>(kAppleTssRootPem.data()), kAppleTssRootPem.size(), CURL_BLOB_COPY };
        TLS_OPT(CURLOPT_CAINFO_BLOB, &root);
#else
        error = "DDI signing requires libcurl 7.77.0 or later with CA bundle support";
        return false;
#endif
#endif
#undef TLS_OPT
        // macOS deliberately keeps the existing system curl/SecureTransport
        // trust store; SecureTransport does not implement CAINFO_BLOB.
        return true;
    }
    catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

std::optional<std::vector<uint8_t>> parse_tss_response(std::string_view response, bool cryptex, std::string& error)
{
    error.clear();
    if (response.empty() || response.size() > kMaxResponse) {
        error = "DDI signing response has an invalid length";
        return { };
    }
    const auto marker = response.find("REQUEST_STRING=");
    const auto prefix = marker == std::string_view::npos ? response : response.substr(0, marker);
    std::optional<unsigned> status;
    std::optional<std::string_view> message;
    size_t offset = 0;
    while (offset < prefix.size()) {
        const auto end = prefix.find('&', offset);
        const auto field = prefix.substr(offset, end == std::string_view::npos ? prefix.size() - offset : end - offset);
        if (field.starts_with("STATUS=")) {
            unsigned number = 0;
            auto text = field.substr(7);
            const auto [last, ec] = std::from_chars(text.data(), text.data() + text.size(), number);
            if (status || ec != std::errc { } || last != text.data() + text.size()) {
                error = "DDI signing response status is malformed";
                return { };
            }
            status = number;
        }
        else if (field.starts_with("MESSAGE=")) {
            if (message) {
                error = "DDI signing response message is malformed";
                return { };
            }
            message = field.substr(8);
        }
        else if (!field.empty()) {
            error = "DDI signing response envelope is malformed";
            return { };
        }
        if (end == std::string_view::npos) {
            break;
        }
        offset = end + 1;
    }
    if (!status || !message) {
        error = "DDI signing response envelope is incomplete";
        return { };
    }
    if (*status != 0 || *message != "SUCCESS") {
        // Do not echo the server MESSAGE: it could contain signing material or arbitrary input.
        error = std::string("Apple rejected DDI signing; status ") + std::to_string(*status);
        return { };
    }
    if (marker == std::string_view::npos || (marker && response[marker - 1] != '&')) {
        error = "DDI signing response has no ticket plist";
        return { };
    }
    std::string parse_error;
    const auto payload = response.substr(marker + std::string_view("REQUEST_STRING=").size());
    auto parsed = plist::parse(payload, &parse_error);
    if (!parsed || !parsed->is_dict()) {
        error = "DDI signing ticket plist is invalid";
        return { };
    }
    const V* ticket = cryptex ? parsed->find("Cryptex1,Ticket") : parsed->find("ApImg4Ticket");
    if (cryptex && (!ticket || (ticket->kind == plist::Kind::Data && ticket->data.empty()))) {
        ticket = parsed->find("ApImg4Ticket");
    }
    if (!ticket || ticket->kind != plist::Kind::Data || ticket->data.empty() || ticket->data.size() > kMaxTicket) {
        error = "DDI signing response contains no valid ticket";
        return { };
    }
    return ticket->data;
}
} // namespace detail

std::optional<std::vector<uint8_t>> request_tss_ticket(const V& request, const TssOptions& options, std::string& error)
{
    error.clear();
    try {
        if (!valid_shape(request)) {
            error = "DDI signing metadata is malformed";
            return std::nullopt;
        }
        if (!request.is_dict()) {
            error = "DDI signing request is not a dictionary";
            return std::nullopt;
        }
        const bool cryptex = truth(request.find("@Cryptex1,Ticket"));
        if (cryptex == truth(request.find("@ApImg4Ticket"))) {
            error = "DDI signing request ticket type is ambiguous";
            return std::nullopt;
        }
        if (options.timeout.count() <= 0) {
            error = "DDI signing timed out";
            return std::nullopt;
        }
        if (options.cancelled && options.cancelled()) {
            error = "DDI signing was cancelled";
            return std::nullopt;
        }
        auto body = plist::write(request);
        if (body.size() > kMaxRequest) {
            error = "DDI signing request exceeds its size limit";
            return std::nullopt;
        }
        static const auto initialized = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (initialized != CURLE_OK) {
            error = "Cannot initialize DDI signing HTTPS transport";
            return std::nullopt;
        }
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
        if (!curl) {
            error = "Cannot create DDI signing HTTPS transport";
            return std::nullopt;
        }
        CurlContext context { { }, &options };
        curl_slist* raw_headers = nullptr;
        for (const char* header : { "Cache-Control: no-cache", "Content-Type: text/xml; charset=utf-8", "Expect:" }) {
            auto* next = curl_slist_append(raw_headers, header);
            if (!next) {
                curl_slist_free_all(raw_headers);
                error = "Cannot create DDI signing HTTPS headers";
                return std::nullopt;
            }
            raw_headers = next;
        }
        std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(raw_headers, curl_slist_free_all);
        const auto timeout = static_cast<long>(std::clamp<int64_t>(options.timeout.count(), 1, std::numeric_limits<long>::max()));
#define TSS_OPT(option, value)                                     \
    if (curl_easy_setopt(curl.get(), option, value) != CURLE_OK) { \
        error = "Cannot configure DDI signing HTTPS transport";    \
        return std::nullopt;                                       \
    }
        if (!detail::configure_tss_https(curl.get(), error)) {
            return { };
        }
        TSS_OPT(CURLOPT_NOSIGNAL, 1L);
        TSS_OPT(CURLOPT_TIMEOUT_MS, timeout);
        TSS_OPT(CURLOPT_CONNECTTIMEOUT_MS, std::min(timeout, 15000L));
        TSS_OPT(CURLOPT_POST, 1L);
        TSS_OPT(CURLOPT_POSTFIELDS, body.data());
        TSS_OPT(CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size()));
        TSS_OPT(CURLOPT_HTTPHEADER, headers.get());
        TSS_OPT(CURLOPT_USERAGENT, "InetURL/1.0");
        TSS_OPT(CURLOPT_WRITEFUNCTION, receive);
        TSS_OPT(CURLOPT_WRITEDATA, &context);
        TSS_OPT(CURLOPT_NOPROGRESS, 0L);
        TSS_OPT(CURLOPT_XFERINFOFUNCTION, progress);
        TSS_OPT(CURLOPT_XFERINFODATA, &context);
#undef TSS_OPT
        const auto result = curl_easy_perform(curl.get());
        if (result != CURLE_OK) {
            if (context.oversized) {
                error = "DDI signing response exceeds its size limit";
            }
            else if (context.callback_failed) {
                error = "DDI signing callback failed";
            }
            else if (context.cancelled) {
                error = "DDI signing was cancelled";
            }
            else if (result == CURLE_OPERATION_TIMEDOUT) {
                error = "DDI signing timed out";
            }
            else {
                error = std::string("DDI signing HTTPS request failed: ") + curl_easy_strerror(result);
            }
            return { };
        }
        long status = 0;
        if (!(curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status) == CURLE_OK && status == 200)) {
            error = "DDI signing server did not return HTTP 200";
            return std::nullopt;
        }
        return detail::parse_tss_response(context.response, cryptex, error);
    }
    catch (const std::exception& e) {
        error = e.what();
        return { };
    }
}
} // namespace maa::ios::ddi
