#include "MaaUtils/Platform.h"
#include "ddi/DownloadInternal.h"
#include "ddi/File.h"
#include "plist/Plist.h"
#include "util/Hex.h"

#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>

namespace maa::ios::ddi
{
namespace
{
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

std::optional<std::vector<uint8_t>> digest_file(const fs::path& path, const EVP_MD* algorithm, std::string& error, bool git_blob = false)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "Cannot open DDI file for checksum";
        return std::nullopt;
    }
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx || EVP_DigestInit_ex(ctx.get(), algorithm, nullptr) != 1) {
        error = "Cannot initialize DDI checksum";
        return std::nullopt;
    }
    if (git_blob) {
        std::string header = "blob " + std::to_string(fs::file_size(path));
        header += '\0';
        if (EVP_DigestUpdate(ctx.get(), header.data(), header.size()) != 1) {
            error = "Cannot calculate DDI checksum";
            return std::nullopt;
        }
    }
    std::array<char, 64 * 1024> buf { };
    while (file) {
        file.read(buf.data(), buf.size());
        if (file.gcount() && EVP_DigestUpdate(ctx.get(), buf.data(), static_cast<size_t>(file.gcount())) != 1) {
            error = "Cannot calculate DDI checksum";
            return std::nullopt;
        }
    }
    if (!file.eof()) {
        error = "Cannot read DDI checksum input";
        return std::nullopt;
    }
    std::vector<uint8_t> result(EVP_MAX_MD_SIZE);
    unsigned length = 0;
    if (EVP_DigestFinal_ex(ctx.get(), result.data(), &length) != 1) {
        error = "Cannot finalize DDI checksum";
        return std::nullopt;
    }
    result.resize(length);
    return result;
}

std::string marker(const detail::Catalog& catalog)
{
    return catalog.build + "\n" + catalog.source_url + "\n";
}

bool hex_digest(std::string_view value, size_t length)
{
    return value.size() == length && util::is_lower_hex(value);
}

bool check_catalog(const detail::Catalog& catalog, std::string& error)
{
    std::vector<std::string_view> expected;
    switch (catalog.kind) {
    case Kind::Classic:
        expected = { "DeveloperDiskImage.dmg", "DeveloperDiskImage.dmg.signature" };
        break;
    case Kind::Personalized:
        expected = { "BuildManifest.plist", "Image.dmg", "Image.dmg.trustcache" };
        break;
    case Kind::Cryptex:
        expected = { "BuildManifest.plist", "Image.dmg", "Image.dmg.trustcache", "Image.dmg.cryptex_info", "Image.dmg.root_hash" };
        break;
    default:
        error = "Invalid DDI download catalog";
        return false;
    }
    bool build_valid =
        !catalog.build.empty()
        && catalog.build.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") == std::string::npos;
    if (catalog.kind == Kind::Classic) {
        std::string version_error;
        const auto version = parse_product_version(catalog.build, version_error);
        build_valid =
            version && version->major < 17 && catalog.build == std::to_string(version->major) + "." + std::to_string(version->minor);
    }
    if (!build_valid || !catalog.source_url.starts_with("https://") || catalog.assets.size() != expected.size()) {
        error = "Invalid DDI download catalog";
        return false;
    }
    for (const auto& asset : catalog.assets) {
        const auto position = std::find(expected.begin(), expected.end(), asset.name);
        if (position == expected.end() || asset.size == 0 || asset.size > 64 * 1024 * 1024
            || (catalog.kind == Kind::Classic ? !hex_digest(asset.git_blob_sha1, 40) : !hex_digest(asset.sha256, 64))
            || (!asset.sha256.empty() && !hex_digest(asset.sha256, 64))
            || (!asset.git_blob_sha1.empty() && !hex_digest(asset.git_blob_sha1, 40))) {
            error = "Invalid DDI download asset";
            return false;
        }
        expected.erase(position); // Reject duplicates, unexpected names and path traversal.
    }
    return true;
}

std::optional<uint64_t> numeric_identity(const plist::Value* value)
{
    if (!value) {
        return std::nullopt;
    }
    if (value->is_int() && value->integer >= 0) {
        return static_cast<uint64_t>(value->integer);
    }
    if (!value->is_string() || value->string.empty()) {
        return std::nullopt;
    }
    std::string_view text = value->string;
    int base = 10;
    if (text.starts_with("0x") || text.starts_with("0X")) {
        base = 16;
        text.remove_prefix(2);
    }
    uint64_t number = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), number, base);
    if (ec != std::errc { } || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return number;
}

bool ascii_equal_fold(std::string_view first, std::string_view second)
{
    if (first.size() != second.size()) {
        return false;
    }
    auto lower = [](unsigned char c) {
        return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
    };
    for (size_t i = 0; i < first.size(); ++i) {
        if (lower(static_cast<unsigned char>(first[i])) != lower(static_cast<unsigned char>(second[i]))) {
            return false;
        }
    }
    return true;
}

bool matches_target(const plist::Value& identity, const DownloadTarget* target)
{
    if (!target) {
        return true;
    }
    const auto* product = identity.find("Ap,ProductType");
    const auto* info = identity.find("Info");
    const auto* model = info ? info->find("DeviceClass") : nullptr;
    return (target->product_type.empty() || (product && product->is_string() && product->string == target->product_type))
           && (target->hardware_model.empty() || (model && model->is_string() && ascii_equal_fold(model->string, target->hardware_model)))
           && (!target->board_id || numeric_identity(identity.find("ApBoardID")) == target->board_id)
           && (!target->chip_id || numeric_identity(identity.find("ApChipID")) == target->chip_id);
}

bool check_manifest(const fs::path& directory, const detail::Catalog& catalog, const DownloadTarget* target, std::string& error)
{
    if (catalog.kind == Kind::Classic) {
        return true;
    }
    const auto contents = detail::read_file(directory / "BuildManifest.plist", plist::kMaxInput, error);
    if (!contents) {
        return false;
    }
    auto manifest = plist::parse(*contents, &error);
    if (!manifest || !manifest->is_dict()) {
        error = "Invalid DDI BuildManifest plist";
        return false;
    }
    const auto* build = manifest->find("ProductBuildVersion");
    const auto* identities = manifest->find("BuildIdentities");
    if (!build || !build->is_string() || build->string != catalog.build || !identities || !identities->is_array()) {
        error = "DDI BuildManifest does not match the pinned build";
        return false;
    }
    const std::vector<std::pair<const char*, const char*>> payloads =
        catalog.kind == Kind::Cryptex
            ? std::vector<std::pair<const char*, const char*>> { { "Cryptex1,GenericDmg", "Image.dmg" },
                                                                 { "Cryptex1,GenericTrustCache", "Image.dmg.trustcache" },
                                                                 { "Cryptex1,CryptexInfoPlist", "Image.dmg.cryptex_info" },
                                                                 { "Cryptex1,GenericVolume", "Image.dmg.root_hash" } }
            : std::vector<std::pair<const char*, const char*>> { { "PersonalizedDMG", "Image.dmg" },
                                                                 { "LoadableTrustCache", "Image.dmg.trustcache" } };
    std::vector<std::vector<uint8_t>> digests;
    for (const auto& [key, name] : payloads) {
        (void)key;
        auto digest = digest_file(directory / name, EVP_sha384(), error);
        if (!digest) {
            return false;
        }
        digests.push_back(std::move(*digest));
    }
    bool found = false;
    bool matched_hardware = false;
    for (const auto& identity : identities->array) {
        const auto* info = identity.find("Info");
        const auto* variant = info ? info->find("Variant") : nullptr;
        if (!variant || !variant->is_string()) {
            continue;
        }
        if (catalog.kind == Kind::Cryptex) {
            if (!variant->string.ends_with("Developer Disk Image Cryptex")) {
                continue;
            }
            // The Cryptex identity is generic; device support is decided by
            // personalization and installation, not SupportedProductTypes.
        }
        else {
            if (!variant->string.ends_with("Developer PDI") || variant->string.find("Cryptex") != std::string::npos
                || !matches_target(identity, target)) {
                continue;
            }
        }
        matched_hardware = true;
        const auto* entries = identity.find("Manifest");
        if (!entries || !entries->is_dict()) {
            if (catalog.kind == Kind::Cryptex) {
                error = "DDI Cryptex manifest entries missing";
                return false;
            }
            continue;
        }
        bool valid = true;
        size_t payload_index = 0;
        for (const auto& [key, name] : payloads) {
            const auto* entry = entries->find(key);
            const auto* digest = entry ? entry->find("Digest") : nullptr;
            const auto* entry_info = entry ? entry->find("Info") : nullptr;
            const auto* path = entry_info ? entry_info->find("Path") : nullptr;
            if (!digest || digest->kind != plist::Kind::Data || digest->data.size() != 48 || !path || !path->is_string()
                || path->string != name || digests[payload_index] != digest->data) {
                if (catalog.kind == Kind::Cryptex) {
                    error = std::string("DDI Cryptex manifest digest mismatch: ") + name;
                    return false;
                }
                valid = false;
                break;
            }
            ++payload_index;
        }
        if (valid) {
            found = true;
        }
    }
    if (!found) {
        if (catalog.kind == Kind::Cryptex) {
            error = "DDI BuildManifest lacks a Cryptex identity";
            return false;
        }
        if (!matched_hardware) {
            error = "DDI BuildManifest lacks a matching Personalized hardware identity";
            return false;
        }
        error = "DDI Personalized manifest digest mismatch for the requested hardware";
        return false;
    }
    return true;
}

bool validate_files(
    const fs::path& directory,
    const detail::Catalog& catalog,
    std::string& error,
    bool complete,
    const DownloadTarget* target)
{
    try {
        if (!check_catalog(catalog, error)) {
            return false;
        }
        if (fs::is_symlink(fs::symlink_status(directory)) || !fs::is_directory(directory)) {
            error = "DDI cache directory is missing or is a symbolic link";
            return false;
        }
        for (const auto& asset : catalog.assets) {
            const auto path = directory / asset.name;
            if (fs::is_symlink(fs::symlink_status(path)) || !fs::is_regular_file(path) || fs::file_size(path) != asset.size) {
                error = std::string("DDI cache size or file type mismatch: ") + asset.name;
                return false;
            }
            if (!asset.sha256.empty()) {
                const auto digest = digest_file(path, EVP_sha256(), error);
                if (!digest) {
                    return false;
                }
                if (util::hex_encode(*digest) != asset.sha256) {
                    error = std::string("DDI cache SHA-256 mismatch: ") + asset.name;
                    return false;
                }
            }
            if (!asset.git_blob_sha1.empty()) {
                const auto digest = digest_file(path, EVP_sha1(), error, true);
                if (!digest) {
                    return false;
                }
                if (util::hex_encode(*digest) != asset.git_blob_sha1) {
                    error = std::string("DDI cache Git blob SHA-1 mismatch: ") + asset.name;
                    return false;
                }
            }
        }
        if (!check_manifest(directory, catalog, target, error)) {
            return false;
        }
        if (complete) {
            const auto contents = detail::read_file(directory / ".complete", 4096, error);
            if (!contents) {
                return false;
            }
            if (*contents != marker(catalog)) {
                error = "DDI cache completion marker mismatch";
                return false;
            }
        }
        error.clear();
        return true;
    }
    catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

struct RemovePath
{
    fs::path path;

    ~RemovePath()
    {
        if (!path.empty()) {
            std::error_code ec;
            fs::remove_all(path, ec);
        }
    }
};

struct CurlContext
{
    const detail::Sink* sink;
    const std::function<bool()>* cancelled;
    bool callback_failed = false;
};

size_t curl_write(char* data, size_t size, size_t count, void* opaque) noexcept
{
    auto& context = *static_cast<CurlContext*>(opaque);
    if (size && count > std::numeric_limits<size_t>::max() / size) {
        return 0;
    }
    const auto length = size * count;
    try {
        if ((*context.sink)(data, length)) {
            return length;
        }
    }
    catch (...) {
        context.callback_failed = true;
    }
    return 0;
}

int curl_progress(void* opaque, curl_off_t, curl_off_t, curl_off_t, curl_off_t) noexcept
{
    auto& context = *static_cast<CurlContext*>(opaque);
    try {
        return (*context.cancelled)() ? 1 : 0;
    }
    catch (...) {
        context.callback_failed = true;
        return 1;
    }
}

bool curl_transfer(
    const std::string& url,
    uint64_t max_bytes,
    std::chrono::milliseconds timeout,
    const std::function<bool()>& cancelled,
    const detail::Sink& sink,
    std::string& error)
{
    static const CURLcode initialized = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (initialized != CURLE_OK) {
        error = "Cannot initialize HTTPS downloader";
        return false;
    }
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
    if (!curl) {
        error = "Cannot create HTTPS downloader";
        return false;
    }
    if (!url.starts_with("https://")) {
        error = "DDI downloads require HTTPS";
        return false;
    }
    CurlContext context { &sink, &cancelled };
    std::array<char, CURL_ERROR_SIZE> buffer { };
    const auto milliseconds = static_cast<long>(std::clamp<int64_t>(timeout.count(), 1, std::numeric_limits<long>::max()));
#define DDI_CURL_OPT(option, value)                                    \
    do {                                                               \
        if (curl_easy_setopt(curl.get(), option, value) != CURLE_OK) { \
            error = "Cannot configure HTTPS downloader";               \
            return false;                                              \
        }                                                              \
    } while (false)
    DDI_CURL_OPT(CURLOPT_URL, url.c_str());
#if LIBCURL_VERSION_NUM >= 0x075500
    DDI_CURL_OPT(CURLOPT_PROTOCOLS_STR, "https");
    DDI_CURL_OPT(CURLOPT_REDIR_PROTOCOLS_STR, "https");
#else
    DDI_CURL_OPT(CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
    DDI_CURL_OPT(CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);
#endif
    DDI_CURL_OPT(CURLOPT_FOLLOWLOCATION, 1L);
    DDI_CURL_OPT(CURLOPT_MAXREDIRS, 3L);
    DDI_CURL_OPT(CURLOPT_SSL_VERIFYPEER, 1L);
    DDI_CURL_OPT(CURLOPT_SSL_VERIFYHOST, 2L);
    DDI_CURL_OPT(CURLOPT_SSLVERSION, static_cast<long>(CURL_SSLVERSION_TLSv1_2));
#ifdef _WIN32
#if defined(CURLSSLOPT_NATIVE_CA)
    // Relocatable Windows bundles use the OS trust store, not the build host's MSYS path.
    DDI_CURL_OPT(CURLOPT_SSL_OPTIONS, static_cast<long>(CURLSSLOPT_NATIVE_CA));
    DDI_CURL_OPT(CURLOPT_CAINFO, static_cast<const char*>(nullptr));
    DDI_CURL_OPT(CURLOPT_CAPATH, static_cast<const char*>(nullptr));
#else
    error = "This Windows libcurl cannot use the native certificate store";
    return false;
#endif
#endif
    DDI_CURL_OPT(CURLOPT_NOSIGNAL, 1L);
    DDI_CURL_OPT(CURLOPT_TIMEOUT_MS, milliseconds);
    DDI_CURL_OPT(CURLOPT_CONNECTTIMEOUT_MS, std::min(milliseconds, 15000L));
    DDI_CURL_OPT(CURLOPT_MAXFILESIZE_LARGE, static_cast<curl_off_t>(max_bytes));
    DDI_CURL_OPT(CURLOPT_FAILONERROR, 1L);
    DDI_CURL_OPT(CURLOPT_USERAGENT, "maa-ios-ddi-downloader/1");
    DDI_CURL_OPT(CURLOPT_ERRORBUFFER, buffer.data());
    DDI_CURL_OPT(CURLOPT_WRITEFUNCTION, curl_write);
    DDI_CURL_OPT(CURLOPT_WRITEDATA, &context);
    DDI_CURL_OPT(CURLOPT_NOPROGRESS, 0L);
    DDI_CURL_OPT(CURLOPT_XFERINFOFUNCTION, curl_progress);
    DDI_CURL_OPT(CURLOPT_XFERINFODATA, &context);
#undef DDI_CURL_OPT
    const auto result = curl_easy_perform(curl.get());
    if (result != CURLE_OK) {
        error = context.callback_failed
                    ? "DDI download callback failed"
                    : std::string("HTTPS DDI download failed: ") + (buffer[0] ? buffer.data() : curl_easy_strerror(result));
        return false;
    }
    long status = 0;
    if (curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status) != CURLE_OK || status != 200) {
        error = "DDI download did not return HTTP 200";
        return false;
    }
    return true;
}

} // namespace

namespace detail
{
const Catalog& cryptex_catalog()
{
    static const Catalog catalog {
        "27A5228h",
        "https://raw.githubusercontent.com/doronz88/DeveloperDiskImage/6eae353ae694bda1c421d4a3eee5459ae59c99a1/PersonalizedImages/"
        "Xcode_iOS_DDI_Cryptex",
        {
            { "BuildManifest.plist", 804946, "27385d7582b03b36bb3104e22b520aee0c47d72fecb4e8ecfe12ef5d966c7012" },
            { "Image.dmg", 15'895'040, "873097f695a8b9734e2abc54f795a8874d40ff6fd11208ecb01ef29534c7c176" },
            { "Image.dmg.trustcache", 1895, "f7f21986074eee03a215aca16ecfc78d6bf183600d8a0d2fb691f9896782e6f0" },
            { "Image.dmg.cryptex_info", 430, "edf49aef55aacc063d4d7be05b713bb545ce2993b3f62bcc15eccd75e610ee6c" },
            { "Image.dmg.root_hash", 229, "3543fad2805b88119695c417e12679380b3b5a2742994bbcc839c8e2de5d7302" },
        }
    };
    return catalog;
}

bool validate(const fs::path& directory, const Catalog& catalog, std::string& error, const DownloadTarget* target)
{
    return validate_files(directory, catalog, error, true, target);
}

std::optional<DownloadResult> download(
    const DownloadOptions& options,
    const Catalog& catalog,
    const Transfer& transfer,
    std::string& error,
    const DownloadTarget* target)
{
    error.clear();
    try {
        if (options.timeout <= std::chrono::seconds::zero()) {
            error = "DDI download timeout must be positive";
            return std::nullopt;
        }
        const auto start = Clock::now();
        // Cap arithmetic before adding to a steady-clock time point.
        if (options.timeout > std::chrono::hours(24)) {
            error = "DDI download timeout exceeds 24 hours";
            return std::nullopt;
        }
        const auto deadline = start + options.timeout;
        auto stopped = [&] {
            if (options.cancelled && options.cancelled()) {
                error = "DDI download cancelled";
                return true;
            }
            if (Clock::now() >= deadline) {
                error = "DDI download timed out";
                return true;
            }
            return false;
        };
        if (stopped()) {
            return std::nullopt;
        }
        fs::path root = options.cache_root;
        std::shared_ptr<void> temporary_storage;
        if (root.empty()) {
            const auto temporary_root = fs::temp_directory_path();
            for (int attempt = 0; attempt != 16; ++attempt) {
                std::array<unsigned char, 16> random;
                if (RAND_bytes(random.data(), static_cast<int>(random.size())) != 1) {
                    error = "Cannot create a unique temporary DDI directory";
                    return std::nullopt;
                }
                const auto name = "maafw-ddi-" + util::hex_encode(random);
                const auto candidate = temporary_root / name;
                if (!fs::create_directory(candidate)) {
                    continue;
                }
                temporary_storage = std::shared_ptr<void>(new fs::path(candidate), [](void* pointer) {
                    std::unique_ptr<fs::path> directory(static_cast<fs::path*>(pointer));
                    std::error_code ignored;
                    fs::remove_all(*directory, ignored);
                });
                root = candidate;
                break;
            }
            if (root.empty()) {
                error = "Cannot create a temporary DDI directory";
                return std::nullopt;
            }
        }
        if (!root.is_absolute()) {
            error = "DDI cache root must be an absolute path";
            return std::nullopt;
        }
        if (!check_catalog(catalog, error)) {
            return std::nullopt;
        }
        fs::create_directories(root);
        if (fs::is_symlink(fs::symlink_status(root))) {
            error = "DDI cache root must not be a symbolic link";
            return std::nullopt;
        }
        const auto cache_name = std::string(kind_name(catalog.kind)) + "-" + catalog.build;
        const auto directory = root / cache_name;
        // Only a complete, fully rehashed cache is reused.
        if (fs::exists(directory) || fs::is_symlink(fs::symlink_status(directory))) {
            if (!validate(directory, catalog, error)) {
                error = std::string("Existing DDI cache is invalid; remove this version directory and retry: ")
                        + MAA_NS::path_to_utf8_string(directory) + " (" + error + ")";
                return std::nullopt;
            }
            if (target && !check_manifest(directory, catalog, target, error)) {
                return std::nullopt;
            }
            if (stopped()) {
                return std::nullopt;
            }
            return DownloadResult { directory, catalog.build, catalog.source_url, true, catalog.kind, temporary_storage };
        }
        const auto lock_path = root / (cache_name + ".lock");
        if (!fs::create_directory(lock_path)) {
            error =
                "Another DDI download owns this version lock; retry after it finishes (remove the lock only if that process has exited)";
            return std::nullopt;
        }
        RemovePath lock { lock_path };
        if (fs::exists(directory)) {
            if (!validate(directory, catalog, error) || stopped()) {
                return std::nullopt;
            }
            if (target && !check_manifest(directory, catalog, target, error)) {
                return std::nullopt;
            }
            return DownloadResult { directory, catalog.build, catalog.source_url, true, catalog.kind, temporary_storage };
        }
        std::vector<uint8_t> nonce(16);
        if (RAND_bytes(nonce.data(), static_cast<int>(nonce.size())) != 1) {
            error = "Cannot generate DDI staging directory name";
            return std::nullopt;
        }
        const auto staging = root / ("." + cache_name + ".partial-" + util::hex_encode(nonce));
        if (!fs::create_directory(staging)) {
            error = "Cannot create unique DDI staging directory";
            return std::nullopt;
        }
        RemovePath cleanup { staging };
        fs::permissions(staging, fs::perms::owner_all, fs::perm_options::replace);
        for (const auto& asset : catalog.assets) {
            if (stopped()) {
                return std::nullopt;
            }
            std::ofstream file(staging / asset.name, std::ios::binary | std::ios::trunc);
            if (!file) {
                error = std::string("Cannot create DDI download file: ") + asset.name;
                return std::nullopt;
            }
            uint64_t received = 0;
            const Sink sink = [&](const char* data, size_t length) {
                if (stopped()) {
                    return false;
                }
                if (length > asset.size - received) {
                    error = std::string("DDI download exceeds pinned file size: ") + asset.name;
                    return false;
                }
                file.write(data, static_cast<std::streamsize>(length));
                if (!file) {
                    error = std::string("Cannot write DDI download file: ") + asset.name;
                    return false;
                }
                received += length;
                if (options.progress) {
                    options.progress(asset.name, received, asset.size);
                }
                return true;
            };
            std::string transfer_error;
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now());
            if (remaining.count() <= 0) {
                error = "DDI download timed out";
                return std::nullopt;
            }
            if (!transfer(catalog.source_url + "/" + asset.name, asset.size, remaining, stopped, sink, transfer_error)) {
                if (error.empty()) {
                    error = std::move(transfer_error);
                }
                if (error.empty()) {
                    error = "DDI download failed";
                }
                return std::nullopt;
            }
            file.flush();
            if (!file) {
                error = std::string("Cannot flush DDI download file: ") + asset.name;
                return std::nullopt;
            }
            file.close();
            if (!file) {
                error = std::string("Cannot close DDI download file: ") + asset.name;
                return std::nullopt;
            }
            if (received != asset.size) {
                error = std::string("Incomplete DDI download: ") + asset.name;
                return std::nullopt;
            }
        }
        if (stopped() || !validate_files(staging, catalog, error, false, target)) {
            return std::nullopt;
        }
        std::ofstream complete(staging / ".complete", std::ios::binary);
        complete << marker(catalog);
        complete.close();
        if (!complete) {
            error = "Cannot write DDI completion marker";
            return std::nullopt;
        }
        if (stopped()) {
            return std::nullopt;
        }
        if (fs::exists(directory)) {
            error = "DDI version directory appeared during download; refusing to overwrite it";
            return std::nullopt;
        }
        fs::rename(staging, directory);
        cleanup.path.clear();
        return DownloadResult { directory, catalog.build, catalog.source_url, false, catalog.kind, temporary_storage };
    }
    catch (const std::exception& e) {
        error = e.what();
        return std::nullopt;
    }
}
} // namespace detail

std::optional<DownloadResult> download_for_target(const DownloadOptions& options, const DownloadTarget& target, std::string& error)
{
    const auto selection = select_for_version(target.system_version, error);
    if (!selection) {
        return std::nullopt;
    }
    const detail::Catalog* catalog = nullptr;
    switch (selection->kind) {
    case Kind::Classic:
        catalog = detail::classic_catalog(selection->version);
        break;
    case Kind::Personalized:
        catalog = &detail::personalized_catalog();
        break;
    case Kind::Cryptex:
        catalog = &detail::cryptex_catalog();
        break;
    }
    if (!catalog) {
        error = "No pinned Classic DDI exists for the requested OS major.minor version";
        return std::nullopt;
    }
    return detail::download(options, *catalog, curl_transfer, error, &target);
}

} // namespace maa::ios::ddi
