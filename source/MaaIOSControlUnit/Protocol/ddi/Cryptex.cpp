#include "ddi/Cryptex.h"
#include "ddi/DownloadInternal.h"
#include "ddi/Tss.h"
#include "i18n/Translation.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <openssl/evp.h>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace maa::ios::ddi
{
namespace
{
constexpr std::string_view service_name = "com.apple.security.cryptexd.remote";
constexpr std::string_view ddi_identifier = "com.apple.MobileAsset.DDI";
using Clock = std::chrono::steady_clock;

const plist::Value& need(const plist::Value& v, std::string_view key)
{
    auto p = v.find(key);
    if (!p) {
        throw std::runtime_error(MAA_IOS_TR("Cryptex manifest is missing a required field"));
    }
    return *p;
}

uint64_t number(const plist::Value& v)
{
    if (v.is_uint()) {
        return v.unsigned_integer;
    }
    if (v.is_int() && v.integer >= 0) {
        return static_cast<uint64_t>(v.integer);
    }
    throw std::runtime_error(MAA_IOS_TR("Cryptex manifest has an invalid integer"));
}

std::string text(const plist::Value& v)
{
    if (!v.is_string() || v.string.empty() || v.string.size() > 256
        || std::any_of(v.string.begin(), v.string.end(), [](unsigned char c) { return c < 32 || c == 127; })) {
        throw std::runtime_error(MAA_IOS_TR("Cryptex manifest has an invalid string"));
    }
    return v.string;
}

std::string read_file(const std::filesystem::path& path, uint64_t limit)
{
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(path)) || !std::filesystem::is_regular_file(path)) {
        throw std::runtime_error(MAA_IOS_TR("Cryptex asset is not a bounded regular file"));
    }
    const auto size = std::filesystem::file_size(path);
    if (size > limit) {
        throw std::runtime_error(MAA_IOS_TR("Cryptex asset is not a bounded regular file"));
    }
    std::string bytes(static_cast<std::size_t>(size), '\0');
    std::ifstream file(path, std::ios::binary);
    file.read(bytes.data(), static_cast<std::streamsize>(size));
    if (!file || static_cast<uint64_t>(file.gcount()) != size) {
        throw std::runtime_error(MAA_IOS_TR("Cannot read the complete Cryptex asset"));
    }
    return bytes;
}

std::string pinned_snapshot(const std::filesystem::path& directory, std::string_view name)
{
    const auto& catalog = detail::cryptex_catalog();
    const auto asset = std::find_if(catalog.assets.begin(), catalog.assets.end(), [&](const auto& a) { return a.name == name; });
    if (asset == catalog.assets.end()) {
        throw std::runtime_error(MAA_IOS_TR("Unknown pinned Cryptex asset"));
    }
    auto bytes = read_file(directory / asset->name, asset->size);
    std::array<uint8_t, EVP_MAX_MD_SIZE> digest { };
    unsigned int length = 0;
    if (bytes.size() != asset->size || EVP_Digest(bytes.data(), bytes.size(), digest.data(), &length, EVP_sha256(), nullptr) != 1
        || length != 32) {
        throw std::runtime_error(MAA_IOS_TR("Cryptex asset snapshot checksum failed"));
    }
    constexpr char hex[] = "0123456789abcdef";
    std::string checksum;
    for (unsigned i = 0; i < length; ++i) {
        checksum += hex[digest[i] >> 4];
        checksum += hex[digest[i] & 15];
    }
    if (checksum != asset->sha256) {
        throw std::runtime_error(MAA_IOS_TR("Cryptex asset changed before installation"));
    }
    return bytes;
}

plist::Value to_plist(const xpc::Value& v, unsigned depth = 0)
{
    if (depth > 32) {
        throw std::runtime_error(MAA_IOS_TR("Cryptex identifiers exceed the nesting limit"));
    }
    switch (v.type) {
    case xpc::Type::Bool:
        return plist::Value::Bool(v.boolean);
    case xpc::Type::Int64:
        return plist::Value::Int(v.int64);
    case xpc::Type::UInt64:
        return plist::Value::UInt(v.uint64);
    case xpc::Type::String:
        return plist::Value::Str(v.string);
    case xpc::Type::Data:
        return plist::Value::OfData(v.data);
    case xpc::Type::Dict: {
        auto r = plist::Value::Dict();
        for (const auto& e : v.dict) {
            r.set(e.key, to_plist(e.value, depth + 1));
        }
        return r;
    }
    default:
        throw std::runtime_error(MAA_IOS_TR("Cryptex identifiers contain an unsupported value"));
    }
}

struct Session
{
    remote::Device& device;
    const CryptexInstallOptions& options;
    Clock::time_point deadline;
    std::stop_source stop;
    std::jthread watcher;

    Session(remote::Device& d, const CryptexInstallOptions& o)
        : device(d)
        , options(o)
        , deadline(Clock::now() + o.timeout)
        , watcher([this](std::stop_token token) {
            while (!token.stop_requested()) {
                if (Clock::now() >= deadline || (options.cancelled && options.cancelled())) {
                    stop.request_stop();
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
        })
    {
    }

    int left(std::string& error)
    {
        if (stop.stop_requested() || (options.cancelled && options.cancelled())) {
            stop.request_stop();
            error = MAA_IOS_TR("Cryptex installation cancelled or timed out");
            return 0;
        }
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now()).count();
        if (ms <= 0) {
            stop.request_stop();
            error = MAA_IOS_TR("Cryptex installation cancelled or timed out");
            return 0;
        }
        return static_cast<int>(std::min<int64_t>(ms, std::numeric_limits<int>::max()));
    }

    std::unique_ptr<remote::ServiceConnection> connect(std::string& error)
    {
        if (!left(error)) {
            return { };
        }
        auto connection = device.connect(service_name, error, false, stop.get_token());
        if (connection && !connection->channel()) {
            error = MAA_IOS_TR("Cryptex service does not use RemoteXPC");
            return { };
        }
        return connection;
    }

    bool invoke(std::string_view routine, const xpc::Value& argv, xpc::Value& result, std::string& error)
    {
        auto connection = connect(error);
        if (!connection) {
            return false;
        }
        auto request = xpc::make_dict();
        xpc::dict_set(request, "routine", xpc::make_string(std::string(routine)));
        xpc::dict_set(request, "argv", argv);
        xpc::Value reply;
        const auto budget = left(error);
        return budget && connection->call(request, reply, budget, error) && detail::unwrap_cryptex_reply(reply, result, error);
    }

    bool installed(std::optional<CryptexInstallResult>& out, std::string& error)
    {
        xpc::Value argv;
        if (!invoke("copy-installed", xpc::make_dict(), argv, error)) {
            return false;
        }
        out.reset();
        if (argv.type == xpc::Type::Null) {
            return true;
        }
        if (!argv.is_dict()) {
            error = MAA_IOS_TR("Cryptex installed state is malformed");
            return false;
        }
        const auto array = argv.find("remote-cryptex-array");
        if (!array) {
            return true;
        }
        if (!array->is_array()) {
            error = MAA_IOS_TR("Cryptex installed state is malformed");
            return false;
        }
        for (const auto& entry : array->array) {
            if (!entry.is_dict()) {
                error = MAA_IOS_TR("Cryptex installed state is malformed");
                return false;
            }
            const auto& id = entry.at("remote-cryptex-identifier");
            const auto& version = entry.at("remote-cryptex-version");
            if (!id.is_string() || !version.is_string() || version.string.empty()) {
                error = MAA_IOS_TR("Cryptex installed state is malformed");
                return false;
            }
            if (id.string != ddi_identifier) {
                continue;
            }
            if (out) {
                error = MAA_IOS_TR("Multiple DDI Cryptex installations require manual resolution");
                return false;
            }
            out = CryptexInstallResult { true, id.string, version.string };
        }
        return true;
    }
};
} // namespace

namespace detail
{
std::optional<CryptexMetadata> parse_cryptex_metadata(const plist::Value& manifest, const plist::Value& info, std::string& error)
{
    error.clear();
    try {
        const auto& identities = need(manifest, "BuildIdentities");
        if (!identities.is_array()) {
            throw std::runtime_error(MAA_IOS_TR("Cryptex manifest identities are invalid"));
        }
        const plist::Value* identity = nullptr;
        for (const auto& candidate : identities.array) {
            const auto metadata = candidate.find("Info");
            const auto variant = metadata ? metadata->find("Variant") : nullptr;
            if (variant && variant->is_string() && variant->string.ends_with("Developer Disk Image Cryptex")) {
                if (identity) {
                    throw std::runtime_error(MAA_IOS_TR("Cryptex manifest has ambiguous identities"));
                }
                identity = &candidate;
            }
        }
        if (!identity) {
            throw std::runtime_error(MAA_IOS_TR("Cryptex manifest lacks its generic identity"));
        }
        CryptexMetadata result;
        result.identity = *identity;
        result.identifier = text(need(info, "CFBundleIdentifier"));
        result.version = text(need(info, "CFBundleVersion"));
        if (result.identifier != ddi_identifier || text(need(info, "RequiredMountPath")) != "/System/Developer") {
            throw std::runtime_error(MAA_IOS_TR("Cryptex asset is not a Developer Disk Image"));
        }
        result.nonce_domain = number(need(*identity, "Cryptex1,NonceDomain"));
        if (result.nonce_domain != 4) {
            throw std::runtime_error(MAA_IOS_TR("Cryptex DDI nonce-domain handle is unsupported"));
        }
        const auto& product_class = need(*identity, "Cryptex1,UseProductClass");
        if (!product_class.is_bool()) {
            throw std::runtime_error(MAA_IOS_TR("Cryptex product-class flag is invalid"));
        }
        auto& p = result.properties;
        p = xpc::make_dict();
        xpc::dict_set(p, "Cryptex1,UseProductClass", xpc::make_bool(product_class.boolean));
        xpc::dict_set(p, "MountedCryptex", xpc::make_bool(false));
        xpc::dict_set(p, "Cryptex1,SubType", xpc::make_uint64(number(need(*identity, "Cryptex1,SubType"))));
        xpc::dict_set(p, "Cryptex1,NonceDomain", xpc::make_uint64(result.nonce_domain));
        xpc::dict_set(p, "Cryptex1,Version", xpc::make_string(text(need(*identity, "Cryptex1,Version"))));
        xpc::dict_set(p, "Cryptex1,PreauthVersion", xpc::make_string(text(need(*identity, "Cryptex1,PreauthorizationVersion"))));
        return result;
    }
    catch (const std::exception& e) {
        error = e.what();
        return { };
    }
}

bool unwrap_cryptex_reply(const xpc::Value& reply, xpc::Value& argv, std::string& error)
{
    error.clear();
    if (!reply.is_dict()) {
        error = MAA_IOS_TR("Cryptex reply is not a dictionary");
        return false;
    }
    if (const auto cferr = reply.find("cferr")) {
        error = MAA_IOS_TR("Cryptex device error");
        if (cferr->is_dict()) {
            const auto& description = cferr->at("cferr_userinfo").at("NSLocalizedDescription");
            if (description.is_string()) {
                std::string safe;
                for (unsigned char c : description.string) {
                    if (safe.size() >= 512) {
                        break;
                    }
                    safe += c < 32 || c == 127 ? '?' : static_cast<char>(c);
                }
                error += ": " + safe;
            }
        }
        return false;
    }
    if (const auto code = reply.find("error")) {
        if ((code->type != xpc::Type::Int64 && code->type != xpc::Type::UInt64)
            || (code->type == xpc::Type::Int64 ? code->int64 != 0 : code->uint64 != 0)) {
            error = MAA_IOS_TR("Cryptex device returned a nonzero or malformed error");
            return false;
        }
    }
    else if (!reply.find("argv")) {
        error = MAA_IOS_TR("Cryptex reply lacks status and payload");
        return false;
    }
    argv = reply.at("argv");
    return true;
}

std::optional<std::vector<uint8_t>> unwrap_cryptex_nonce(const std::vector<uint8_t>& blob, std::string& error)
{
    error.clear();
    if (blob.size() != 56) {
        error = MAA_IOS_TR("Cryptex nonce structure has an invalid size");
        return { };
    }
    uint32_t n = 0;
    for (unsigned i = 0; i < 4; ++i) {
        n |= static_cast<uint32_t>(blob[blob.size() - 4 + i]) << (i * 8);
    }
    if (n != 48 || n > blob.size() - 6) {
        error = MAA_IOS_TR("Cryptex nonce length is invalid");
        return { };
    }
    return std::vector<uint8_t>(blob.begin() + 2, blob.begin() + 2 + n);
}

xpc::Value cryptex_install_request(const CryptexMetadata& metadata, const std::array<uint64_t, 5>& sizes)
{
    auto argv = xpc::make_dict();
    for (const auto& [key, value] : std::array<std::pair<const char*, uint64_t>, 4> {
             { { "auth", 0 }, { "client-version", 3 }, { "nonce-persistence", 1 }, { "persistence", 2 } } }) {
        xpc::dict_set(argv, key, xpc::make_uint64(value));
    }
    xpc::dict_set(argv, "image-type-index", xpc::make_int64(10));
    xpc::dict_set(argv, "cryptex1-properties", metadata.properties);
    constexpr std::array<const char*, 5> keys { "image", "trustcache", "im4m", "info", "volumehash" };
    for (std::size_t i = 0; i < keys.size(); ++i) {
        xpc::dict_set(argv, keys[i], xpc::make_file_transfer(sizes[i], i + 1));
    }
    auto request = xpc::make_dict();
    xpc::dict_set(request, "routine", xpc::make_string("install"));
    xpc::dict_set(request, "argv", std::move(argv));
    return request;
}
} // namespace detail

bool query_installed_cryptex(
    remote::Device& device,
    std::optional<CryptexInstallResult>& result,
    const CryptexInstallOptions& options,
    std::string& error)
{
    error.clear();
    result.reset();
    try {
        if (options.timeout.count() <= 0 || options.timeout > std::chrono::hours(1)) {
            error = MAA_IOS_TR("Cryptex installation timeout is invalid");
            return false;
        }
        Session session(device, options);
        if (!session.installed(result, error)) {
            error = std::string(MAA_IOS_TR("Cryptex installed-state query failed: ")) + error;
            return false;
        }
        return true;
    }
    catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

std::optional<CryptexInstallResult> install_cryptex(
    remote::Device& device,
    const DownloadResult& download,
    const DownloadTarget& target,
    const CryptexInstallOptions& options,
    std::string& error)
{
    error.clear();
    try {
        const auto selection = select_for_version(target.system_version, error);
        if (!selection || selection->kind != Kind::Cryptex || download.kind != Kind::Cryptex) {
            if (error.empty()) {
                error = MAA_IOS_TR("Cryptex installation requires a supported Cryptex OS target");
            }
            return { };
        }
        if (options.timeout.count() <= 0 || options.timeout > std::chrono::hours(1)) {
            error = MAA_IOS_TR("Cryptex installation timeout is invalid");
            return { };
        }
        Session session(device, options);
        if (!session.left(error)) {
            return { };
        }
        if (!device.rsd().supports(service_name, "CryptexInstall")) {
            error = MAA_IOS_TR("Device does not advertise CryptexInstall");
            return { };
        }
        if (!detail::validate(download.directory, detail::cryptex_catalog(), error, &target)) {
            return { };
        }
        auto manifest = plist::parse(pinned_snapshot(download.directory, "BuildManifest.plist"), &error);
        auto info = plist::parse(pinned_snapshot(download.directory, "Image.dmg.cryptex_info"), &error);
        if (!manifest || !info) {
            return { };
        }
        auto metadata = detail::parse_cryptex_metadata(*manifest, *info, error);
        if (!metadata) {
            return { };
        }
        std::optional<CryptexInstallResult> existing;
        if (!session.installed(existing, error)) {
            return { };
        }
        if (existing && existing->version == metadata->version && !options.replace_existing) {
            return existing;
        }
        if (existing && !options.replace_existing) {
            error = MAA_IOS_TR("A different DDI Cryptex is installed; explicit replacement is required");
            return { };
        }
        std::optional<bool> personalized = false;
        // A Cryptex DDI is also reported as Personalized by the image mounter.
        // Only inspect that path when cryptexd has no DDI installed.
        if (!existing) {
            if (!options.personalized_mounted) {
                error = MAA_IOS_TR("Personalized DDI mount inspection is required before Cryptex installation");
                return { };
            }
            const auto budget = session.left(error);
            if (!budget) {
                return { };
            }
            personalized = options.personalized_mounted(std::chrono::milliseconds(budget), error);
            if (!personalized) {
                return { };
            }
        }
        if (*personalized && (!options.replace_existing || !options.remove_personalized)) {
            error = MAA_IOS_TR("A Personalized DDI is mounted; explicit replacement is required");
            return { };
        }
        if (!device.rsd().supports(service_name, "ReadIdentifiers")) {
            error = MAA_IOS_TR("Device does not advertise Cryptex ReadIdentifiers");
            return { };
        }
        xpc::Value identifiers;
        if (!session.invoke("read-personalization-id", xpc::make_dict(), identifiers, error)) {
            return { };
        }
        if (!identifiers.is_dict()) {
            error = MAA_IOS_TR("Cryptex personalization identifiers are invalid");
            return { };
        }
        auto nonce_request = xpc::make_dict();
        xpc::dict_set(nonce_request, "nonce-domain-handle", xpc::make_uint64(metadata->nonce_domain));
        xpc::Value nonce_reply;
        if (!session.invoke("get-nonce", nonce_request, nonce_reply, error)) {
            return { };
        }
        const auto& nonce_blob = nonce_reply.at("nonce");
        if (nonce_blob.type != xpc::Type::Data) {
            error = MAA_IOS_TR("Cryptex nonce reply is invalid");
            return { };
        }
        auto nonce = detail::unwrap_cryptex_nonce(nonce_blob.data, error);
        if (!nonce) {
            return { };
        }
        auto tss = build_cryptex_tss_request(metadata->identity, to_plist(identifiers), *nonce, error);
        if (!tss) {
            return { };
        }
        TssOptions tss_options;
        const auto tss_budget = session.left(error);
        if (!tss_budget) {
            return { };
        }
        tss_options.timeout = std::chrono::milliseconds(tss_budget);
        tss_options.cancelled = [&session] {
            return session.stop.stop_requested();
        };
        auto ticket = request_tss_ticket(*tss, tss_options, error);
        if (!ticket || ticket->empty() || ticket->size() > (1u << 20)) {
            if (error.empty()) {
                error = MAA_IOS_TR("Cryptex ticket size is invalid");
            }
            return { };
        }
        // Use bounded, independently hashed memory snapshots for the actual
        // uploads. Replacing/modifying cache paths cannot change sent bytes after
        // a checksum succeeds. Prepare all assets before any uninstall mutation.
        if (!session.left(error)) {
            return { };
        }
        constexpr std::array<const char*, 4> names { "Image.dmg", "Image.dmg.trustcache", "Image.dmg.cryptex_info", "Image.dmg.root_hash" };
        std::array<std::istringstream, 4> files;
        std::array<uint64_t, 5> sizes { };
        for (std::size_t i = 0; i < files.size(); ++i) {
            auto snapshot = pinned_snapshot(download.directory, names[i]);
            sizes[i < 2 ? i : i + 1] = snapshot.size();
            files[i].str(std::move(snapshot));
        }
        sizes[2] = ticket->size();
        // Signing is an HTTP round trip. A reboot or another installer may
        // change the live nonce meanwhile; reject the stale ticket before
        // removing any existing DDI or sending an installation request.
        xpc::Value current_nonce_reply;
        if (!session.invoke("get-nonce", nonce_request, current_nonce_reply, error)) {
            return { };
        }
        const auto& current_blob = current_nonce_reply.at("nonce");
        if (current_blob.type != xpc::Type::Data) {
            error = MAA_IOS_TR("Cryptex nonce reply is invalid");
            return { };
        }
        const auto current_nonce = detail::unwrap_cryptex_nonce(current_blob.data, error);
        if (!current_nonce) {
            return { };
        }
        if (*current_nonce != *nonce) {
            error = MAA_IOS_TR("Cryptex personalization nonce changed before installation");
            return { };
        }
        if (existing) {
            auto argv = xpc::make_dict();
            xpc::dict_set(argv, "remote-cryptex-identifier", xpc::make_string(std::string(ddi_identifier)));
            xpc::dict_set(argv, "remote-cryptex-version", xpc::make_string(existing->version));
            xpc::Value ignored;
            if (!session.invoke("uninstall", argv, ignored, error)) {
                return { };
            }
        }
        if (*personalized) {
            const auto left = session.left(error);
            if (!left || !options.remove_personalized(std::chrono::milliseconds(left), error)) {
                return { };
            }
        }
        auto connection = session.connect(error);
        if (!connection) {
            return { };
        }
        auto* channel = connection->channel();
        if (!channel->send_request(detail::cryptex_install_request(*metadata, sizes), true, error)) {
            return { };
        }
        std::istringstream ticket_stream(std::string(reinterpret_cast<const char*>(ticket->data()), ticket->size()));
        std::array<std::istream*, 5> streams { &files[0], &files[1], &ticket_stream, &files[2], &files[3] };
        constexpr std::array<const char*, 5> labels { "Image.dmg",
                                                      "Image.dmg.trustcache",
                                                      "Cryptex1 ticket",
                                                      "Image.dmg.cryptex_info",
                                                      "Image.dmg.root_hash" };
        for (std::size_t i = 0; i < streams.size(); ++i) {
            const auto left = session.left(error);
            if (!left
                || !channel->send_file_transfer(
                    i + 1,
                    sizes[i],
                    *streams[i],
                    left,
                    [&session] { return session.stop.stop_requested(); },
                    [&](uint64_t current, uint64_t total) {
                        if (options.progress) {
                            options.progress(labels[i], current, total);
                        }
                    },
                    error)) {
                return { };
            }
        }
        xpc::Value reply, ignored;
        const auto left = session.left(error);
        if (!left || !channel->receive(reply, left, error) || !detail::unwrap_cryptex_reply(reply, ignored, error)) {
            error = std::string(MAA_IOS_TR("Cryptex final installation reply failed: ")) + error;
            return { };
        }
        connection.reset();
        std::optional<CryptexInstallResult> installed;
        if (!session.installed(installed, error)) {
            error = std::string(MAA_IOS_TR("Cryptex post-installation verification failed: ")) + error;
            return { };
        }
        if (!installed || installed->identifier != metadata->identifier || installed->version != metadata->version) {
            error = MAA_IOS_TR("Cryptex installation reply succeeded but the expected DDI is not installed");
            return { };
        }
        installed->already_installed = false;
        return installed;
    }
    catch (const std::exception& e) {
        error = e.what();
        return { };
    }
}
} // namespace maa::ios::ddi
