#include "ddi/Mounter.h"
#include "ddi/DownloadInternal.h"
#include <algorithm>
#include <array>
#include <exception>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <openssl/evp.h>
#include <tuple>

namespace maa::ios::ddi
{
namespace
{
using V = plist::Value;
using Clock = std::chrono::steady_clock;
constexpr size_t max_file = 64 * 1024 * 1024;
constexpr size_t max_ticket = 256 * 1024;

const char* image_type(Kind kind)
{
    return kind == Kind::Classic ? "Developer" : "Personalized";
}

std::string safe_text(std::string_view text)
{
    std::string out;
    for (unsigned char c : text.substr(0, 512)) {
        out += c >= 32 && c != 127 ? static_cast<char>(c) : '?';
    }
    return out;
}

bool bounded_value(const V& value, size_t depth, size_t& budget, size_t& nodes)
{
    if (depth > 32 || ++nodes > 8192) {
        return false;
    }
    size_t size = value.string.size() + value.data.size();
    for (const auto& key : value.keys) {
        if (key.size() > budget) {
            return false;
        }
        budget -= key.size();
    }
    if (size > budget) {
        return false;
    }
    budget -= size;
    for (const auto& child : value.values) {
        if (!bounded_value(child, depth + 1, budget, nodes)) {
            return false;
        }
    }
    for (const auto& child : value.array) {
        if (!bounded_value(child, depth + 1, budget, nodes)) {
            return false;
        }
    }
    return true;
}

struct Session
{
    PlistService& service;
    const MountOptions& options;
    std::string& error;
    Clock::time_point deadline;
    bool valid = false;
    bool copy_unsupported = false;

    Session(PlistService& s, const MountOptions& o, std::string& e)
        : service(s)
        , options(o)
        , error(e)
    {
        if (o.timeout <= std::chrono::seconds::zero() || o.timeout > std::chrono::hours(24)) {
            error = "DDI install timeout must be positive and at most 24 hours";
            return;
        }
        deadline = Clock::now() + o.timeout;
        valid = true;
    }

    std::chrono::milliseconds remaining() const
    {
        if (!valid) {
            return std::chrono::milliseconds::zero();
        }
        if (options.cancelled && options.cancelled()) {
            error = "DDI install cancelled";
            return std::chrono::milliseconds::zero();
        }
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now());
        if (left <= std::chrono::milliseconds::zero()) {
            error = "DDI install timed out";
            return std::chrono::milliseconds::zero();
        }
        return left;
    }

    std::optional<V> check_reply(std::optional<V> reply)
    {
        if (!remaining().count()) {
            return std::nullopt;
        }
        if (!reply) {
            error = error.empty() ? "DDI service disconnected without a reply" : safe_text(error);
            return std::nullopt;
        }
        size_t budget = 2 * 1024 * 1024, nodes = 0;
        if (!reply->is_dict() || !bounded_value(*reply, 0, budget, nodes)) {
            error = "Invalid or oversized DDI service reply";
            return std::nullopt;
        }
        return reply;
    }

    std::optional<V> request(const V& request)
    {
        if (!service.request) {
            error = "DDI service request callback is missing";
            return std::nullopt;
        }
        const auto left = remaining();
        if (!left.count()) {
            return std::nullopt;
        }
        return check_reply(service.request(request, left, error));
    }

    bool reconnect()
    {
        if (!service.reconnect) {
            error = "DDI service reconnect callback is missing";
            return false;
        }
        const auto left = remaining();
        if (!left.count()) {
            return false;
        }
        if (!service.reconnect(left, error)) {
            error = error.empty() ? "Cannot reconnect DDI service" : safe_text(error);
            return false;
        }
        return remaining().count() > 0;
    }
};

V command(std::string name)
{
    auto v = V::Dict();
    v.set("Command", V::Str(std::move(name)));
    return v;
}

std::string device_error(const V& reply)
{
    const auto* e = reply.find("Error");
    if (!e) {
        return { };
    }
    if (!e->is_string() || e->string.empty()) {
        return "Invalid DDI service error field";
    }
    std::string text = safe_text(e->string);
    if (auto* detail = reply.find("DetailedError"); detail && detail->is_string()) {
        text += ": " + safe_text(detail->string);
    }
    return text;
}

bool successful(const std::optional<V>& reply, std::string_view status, std::string& error)
{
    if (!reply) {
        return false;
    }
    error = device_error(*reply);
    if (!error.empty()) {
        return false;
    }
    auto* value = reply->find("Status");
    if (!value || !value->is_string() || value->string != status) {
        error = std::string("Unexpected DDI service status; expected ") + std::string(status);
        return false;
    }
    return true;
}

std::optional<MountResult> inspect(Session& s, Kind kind, std::string& error)
{
    MountResult result;
    result.image_type = image_type(kind);
    auto request = command("LookupImage");
    request.set("ImageType", V::Str(result.image_type));
    auto reply = s.request(request);
    if (!reply) {
        return std::nullopt;
    }
    error = device_error(*reply);
    if (!error.empty()) {
        return std::nullopt;
    }
    auto* present = reply->find("ImagePresent");
    if (present && !present->is_bool()) {
        error = "Invalid DDI ImagePresent field";
        return std::nullopt;
    }
    auto* signature = reply->find("ImageSignature");
    if (signature && signature->is_array()) {
        if (signature->array.size() > 128) {
            error = "Too many mounted DDI signatures";
            return std::nullopt;
        }
        for (const auto& entry : signature->array) {
            if (entry.kind != plist::Kind::Data || entry.data.size() > max_ticket) {
                error = "Invalid mounted DDI signature";
                return std::nullopt;
            }
        }
        if (!signature->array.empty()) {
            signature = &signature->array.front();
        }
        else {
            signature = nullptr;
        }
    }
    if (signature && (signature->kind != plist::Kind::Data || signature->data.size() > max_ticket)) {
        error = "Invalid mounted DDI signature";
        return std::nullopt;
    }
    if (signature && !signature->data.empty()) {
        if (present && !present->boolean) {
            error = "Contradictory DDI lookup reply";
            return std::nullopt;
        }
        result.state = MountState::AlreadyMounted;
        result.signature = signature->data;
        result.verification = "LookupImage signature";
        return result;
    }
    // Empty signatures occur for mounted Personalized DDIs. Do not infer
    // presence from ImagePresent alone; require a mounted CopyDevices entry.
    if (!s.copy_unsupported) {
        auto devices = s.request(command("CopyDevices"));
        if (!devices) {
            return std::nullopt;
        }
        auto* entries = devices->find("EntryList");
        auto* e = devices->find("Error");
        bool unknown = e && e->is_string() && (e->string == "UnknownCommand" || e->string == "UnsupportedCommand");
        if (unknown) {
            s.copy_unsupported = true;
            if (!s.reconnect()) {
                return std::nullopt;
            }
        }
        else {
            auto detail = device_error(*devices);
            if (!detail.empty()) {
                error = detail;
                return std::nullopt;
            }
            if (!entries || !entries->is_array() || entries->array.size() > 1024) {
                error = "Invalid DDI CopyDevices reply";
                return std::nullopt;
            }
            for (const auto& entry : entries->array) {
                if (!entry.is_dict()) {
                    error = "Invalid DDI mounted image entry";
                    return std::nullopt;
                }
                auto* type = entry.find("DiskImageType");
                if (type && type->is_string() && type->string == result.image_type) {
                    // CopyDevices can describe attached disk images. Their
                    // type alone does not establish a mounted developer image.
                    const auto* mounted = entry.find("IsMounted");
                    if (!mounted || !mounted->is_bool()) {
                        error = "Invalid DDI mounted image entry";
                        return std::nullopt;
                    }
                    if (!mounted->boolean) {
                        continue;
                    }
                    const auto* path = entry.find("MountPath");
                    const std::string_view expected = kind == Kind::Classic ? "/Developer" : "/System/Developer";
                    if (!path || !path->is_string() || path->string != expected) {
                        error = "Invalid DDI mounted image entry";
                        return std::nullopt;
                    }
                    result.state = MountState::AlreadyMounted;
                    result.verification = "CopyDevices entry";
                    return result;
                }
            }
        }
    }
    if (present && present->boolean) {
        error = "DDI lookup claims presence without verifiable mounted image";
        return std::nullopt;
    }
    return result;
}

bool developer_mode(Session& s, uint32_t major, std::string& error)
{
    // Pre-iOS16 services do not support the Developer Mode query.
    if (major < 16) {
        return true;
    }
    auto reply = s.request(command("QueryDeveloperModeStatus"));
    if (!reply) {
        return false;
    }
    error = device_error(*reply);
    if (!error.empty()) {
        return false;
    }
    const auto* enabled = reply->find("DeveloperModeStatus");
    if (!enabled || !enabled->is_bool()) {
        error = "Cannot verify device Developer Mode";
        return false;
    }
    if (!enabled->boolean) {
        error = "Enable Developer Mode on the device before installing DDI";
        return false;
    }
    return true;
}

bool unmount(Session& s, Kind kind, std::string& error)
{
    auto request = command("UnmountImage");
    request.set("MountPath", V::Str(kind == Kind::Classic ? "/Developer" : "/System/Developer"));
    if (!successful(s.request(request), "Complete", error)) {
        return false;
    }
    const auto result = inspect(s, kind, error);
    if (!result) {
        return false;
    }
    if (result->state != MountState::NotMounted) {
        error = "DDI remains mounted after unmount request";
        return false;
    }
    return true;
}

std::optional<std::vector<uint8_t>> hash_bytes(const std::vector<uint8_t>& data, const EVP_MD* algorithm, bool git, std::string& error)
{
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx || EVP_DigestInit_ex(ctx.get(), algorithm, nullptr) != 1) {
        error = "Cannot hash DDI asset";
        return std::nullopt;
    }
    if (git) {
        auto prefix = "blob " + std::to_string(data.size());
        prefix.push_back('\0');
        if (EVP_DigestUpdate(ctx.get(), prefix.data(), prefix.size()) != 1) {
            error = "Cannot hash DDI asset";
            return std::nullopt;
        }
    }
    std::vector<uint8_t> output(EVP_MAX_MD_SIZE);
    unsigned size = 0;
    if (EVP_DigestUpdate(ctx.get(), data.data(), data.size()) != 1 || EVP_DigestFinal_ex(ctx.get(), output.data(), &size) != 1) {
        error = "Cannot hash DDI asset";
        return std::nullopt;
    }
    output.resize(size);
    return output;
}

std::string hex(const std::vector<uint8_t>& data)
{
    std::string text;
    for (auto b : data) {
        text += "0123456789abcdef"[b >> 4];
        text += "0123456789abcdef"[b & 15];
    }
    return text;
}

std::optional<std::map<std::string, std::vector<uint8_t>>>
    load_assets(Session& s, const DownloadResult& download, const detail::Catalog& catalog, std::string& error)
{
    std::map<std::string, std::vector<uint8_t>> assets;
    for (const auto& asset : catalog.assets) {
        if (!s.remaining().count()) {
            return std::nullopt;
        }
        if (asset.size > max_file) {
            error = "DDI asset exceeds install size limit";
            return std::nullopt;
        }
        std::ifstream file(download.directory / asset.name, std::ios::binary);
        std::vector<uint8_t> bytes(static_cast<size_t>(asset.size));
        if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())) || file.peek() != EOF) {
            error = "Cannot read complete DDI install asset";
            return std::nullopt;
        }
        // Upload only the independently checked memory snapshot.
        for (const auto& [algorithm, expected, git] :
             { std::tuple { EVP_sha256(), asset.sha256, false }, std::tuple { EVP_sha1(), asset.git_blob_sha1, true } }) {
            if (expected.empty()) {
                continue;
            }
            const auto digest = hash_bytes(bytes, algorithm, git, error);
            if (!digest) {
                return std::nullopt;
            }
            if (hex(*digest) != expected) {
                error = "DDI install asset changed after cache validation";
                return std::nullopt;
            }
        }
        assets.emplace(asset.name, std::move(bytes));
    }
    return assets;
}

std::optional<uint64_t> unsigned_number(const V* value)
{
    if (value && value->is_int() && value->integer >= 0) {
        return static_cast<uint64_t>(value->integer);
    }
    if (value && value->is_uint()) {
        return value->unsigned_integer;
    }
    return std::nullopt;
}
} // namespace

std::optional<MountResult> inspect_mounted(PlistService& service, Kind kind, const MountOptions& options, std::string& error)
{
    error.clear();
    try {
        Session session(service, options, error);
        return inspect(session, kind, error);
    }
    catch (const std::exception& e) {
        error = safe_text(e.what());
        return std::nullopt;
    }
}

std::optional<MountResult>
    inspect_mounted(PlistService& service, const DownloadTarget& target, const MountOptions& options, std::string& error)
{
    const auto selected = select_for_version(target.system_version, error);
    if (!selected) {
        return std::nullopt;
    }
    auto mounted = inspect_mounted(service, selected->kind, options, error);
    if (!mounted || mounted->state == MountState::NotMounted || selected->kind != Kind::Classic) {
        return mounted;
    }
    const auto* catalog = detail::classic_catalog(selected->version);
    if (!catalog) {
        error = "No pinned Classic DDI exists for the requested OS major.minor version";
        return std::nullopt;
    }
    const auto asset = std::ranges::find(catalog->assets, "DeveloperDiskImage.dmg.signature", &detail::Asset::name);
    if (asset == catalog->assets.end() || (asset->sha256.empty() && asset->git_blob_sha1.empty())) {
        error = "Classic DDI catalog has no pinned signature digest";
        return std::nullopt;
    }
    try {
        mounted->signature_matches = mounted->signature.size() == asset->size;
        for (const auto& [algorithm, expected, git] :
             { std::tuple { EVP_sha256(), asset->sha256, false }, std::tuple { EVP_sha1(), asset->git_blob_sha1, true } }) {
            if (expected.empty() || !*mounted->signature_matches) {
                continue;
            }
            const auto digest = hash_bytes(mounted->signature, algorithm, git, error);
            if (!digest) {
                return std::nullopt;
            }
            mounted->signature_matches = hex(*digest) == expected;
        }
        return mounted;
    }
    catch (const std::exception& e) {
        error = safe_text(e.what());
        return std::nullopt;
    }
}

bool unmount_current(PlistService& service, Kind kind, const MountOptions& options, std::string& error)
{
    error.clear();
    try {
        Session session(service, options, error);
        return unmount(session, kind, error);
    }
    catch (const std::exception& e) {
        error = safe_text(e.what());
        return false;
    }
}

namespace detail
{
std::optional<MountResult> mount_image_with_catalog(
    PlistService& service,
    const DownloadResult& download,
    const DownloadTarget& target,
    uint64_t ecid,
    const MountOptions& options,
    const Catalog& catalog,
    std::string& error)
{
    error.clear();
    try {
        Session s(service, options, error);
        if (!s.remaining().count()) {
            return std::nullopt;
        }
        if (catalog.kind == Kind::Cryptex) {
            error = "Cryptex DDI requires the cryptexd installer";
            return std::nullopt;
        }
        if (download.kind != catalog.kind || download.build != catalog.build) {
            error = "DDI install result does not match its catalog";
            return std::nullopt;
        }
        auto version = parse_product_version(target.system_version, error);
        auto selected = version ? select_for_version(*version, error) : std::nullopt;
        if (!selected || selected->kind != catalog.kind) {
            error = "DDI install category does not match device OS";
            return std::nullopt;
        }
        if (!validate(download.directory, catalog, error, &target)) {
            return std::nullopt;
        }
        auto assets = load_assets(s, download, catalog, error);
        if (!assets) {
            return std::nullopt;
        }
        auto existing = inspect(s, catalog.kind, error);
        if (!existing) {
            return std::nullopt;
        }
        if (existing->state == MountState::AlreadyMounted && !options.replace_existing) {
            if (catalog.kind == Kind::Classic) {
                existing->signature_matches = existing->signature == assets->at("DeveloperDiskImage.dmg.signature");
                if (!*existing->signature_matches) {
                    error = "Mounted Classic DDI signature does not match the requested image; enable replacement explicitly";
                    return std::nullopt;
                }
            }
            return existing;
        }
        if (!developer_mode(s, version->major, error)) {
            return std::nullopt;
        }
        std::vector<uint8_t> signature;
        const auto& image = assets->at(catalog.kind == Kind::Classic ? "DeveloperDiskImage.dmg" : "Image.dmg");
        if (catalog.kind == Kind::Classic) {
            signature = assets->at("DeveloperDiskImage.dmg.signature");
        }
        else {
            if (!options.personalize || !ecid) {
                error = "DDI personalization requires a ticket callback and device ECID";
                return std::nullopt;
            }
            auto identifiers_reply = s.request(command("QueryPersonalizationIdentifiers"));
            if (!identifiers_reply) {
                return std::nullopt;
            }
            auto e = device_error(*identifiers_reply);
            if (!e.empty()) {
                error = e;
                return std::nullopt;
            }
            auto* identifiers = identifiers_reply->find("PersonalizationIdentifiers");
            auto board = identifiers ? unsigned_number(identifiers->find("BoardId")) : std::nullopt;
            auto chip = identifiers ? unsigned_number(identifiers->find("ChipID")) : std::nullopt;
            if (!identifiers || !identifiers->is_dict() || !board || !chip) {
                error = "Invalid DDI personalization identifiers";
                return std::nullopt;
            }
            if ((target.board_id && target.board_id != board) || (target.chip_id && target.chip_id != chip)) {
                error = "DDI device hardware identity changed";
                return std::nullopt;
            }
            DownloadTarget confirmed = target;
            confirmed.board_id = board;
            confirmed.chip_id = chip;
            if (!validate(download.directory, catalog, error, &confirmed)) {
                return std::nullopt;
            }
            auto nonce_request = command("QueryNonce");
            nonce_request.set("PersonalizedImageType", V::Str("DeveloperDiskImage"));
            auto nonce_reply = s.request(nonce_request);
            if (!nonce_reply) {
                return std::nullopt;
            }
            e = device_error(*nonce_reply);
            if (!e.empty()) {
                error = e;
                return std::nullopt;
            }
            auto* nonce = nonce_reply->find("PersonalizationNonce");
            if (!nonce || nonce->kind != plist::Kind::Data || nonce->data.empty() || nonce->data.size() > 128) {
                error = "Invalid DDI personalization nonce";
                return std::nullopt;
            }
            auto& manifest_bytes = assets->at("BuildManifest.plist");
            auto manifest =
                plist::parse(std::string_view(reinterpret_cast<const char*>(manifest_bytes.data()), manifest_bytes.size()), &error);
            if (!manifest) {
                error = "Cannot parse DDI install manifest";
                return std::nullopt;
            }
            const auto left = s.remaining();
            if (!left.count()) {
                return std::nullopt;
            }
            auto ticket = options.personalize(*manifest, *identifiers, ecid, nonce->data, left, error);
            if (!s.remaining().count()) {
                return std::nullopt;
            }
            if (!ticket) {
                error = error.empty() ? "DDI personalization did not return a ticket" : safe_text(error);
                return std::nullopt;
            }
            signature = std::move(*ticket);
            // Recheck the live nonce after HTTP. A reboot/other installer must
            // not cause this connection to upload a ticket for an earlier nonce.
            auto current = s.request(nonce_request);
            if (!current) {
                return std::nullopt;
            }
            e = device_error(*current);
            if (!e.empty()) {
                error = e;
                return std::nullopt;
            }
            auto* current_nonce = current->find("PersonalizationNonce");
            if (!current_nonce || current_nonce->kind != plist::Kind::Data || current_nonce->data != nonce->data) {
                error = "DDI personalization nonce changed before upload";
                return std::nullopt;
            }
        }
        if (signature.empty() || signature.size() > max_ticket) {
            error = "Invalid DDI install signature or ticket";
            return std::nullopt;
        }
        if (existing->state == MountState::AlreadyMounted) {
            if (!unmount(s, catalog.kind, error)) {
                return std::nullopt;
            }
        }
        auto upload = command("ReceiveBytes");
        upload.set("ImageType", V::Str(image_type(catalog.kind)));
        upload.set("ImageSize", V::Int(static_cast<int64_t>(image.size())));
        upload.set("ImageSignature", V::OfData(signature));
        if (!successful(s.request(upload), "ReceiveBytesAck", error)) {
            return std::nullopt;
        }
        if (!service.send_bytes || !service.receive) {
            error = "DDI image transfer callbacks are missing";
            return std::nullopt;
        }
        for (size_t offset = 0; offset < image.size();) {
            auto amount = std::min<size_t>(64 * 1024, image.size() - offset);
            const auto left = s.remaining();
            if (!left.count()) {
                return std::nullopt;
            }
            if (!service.send_bytes(image.data() + offset, amount, left, error)) {
                error = error.empty() ? "DDI image upload failed" : safe_text(error);
                return std::nullopt;
            }
            offset += amount;
            if (!s.remaining().count()) {
                return std::nullopt;
            }
            if (options.progress) {
                options.progress(offset, image.size());
            }
        }
        const auto left = s.remaining();
        if (!left.count() || !successful(s.check_reply(service.receive(left, error)), "Complete", error)) {
            return std::nullopt;
        }
        // A concurrent installer must not be mistaken for our own mount.
        const auto before_mount = inspect(s, catalog.kind, error);
        if (!before_mount) {
            return std::nullopt;
        }
        if (before_mount->state != MountState::NotMounted) {
            error = "Another DDI became mounted during upload";
            return std::nullopt;
        }
        auto mount = command("MountImage");
        mount.set("ImageType", V::Str(image_type(catalog.kind)));
        mount.set("ImageSignature", V::OfData(signature));
        if (catalog.kind == Kind::Personalized) {
            mount.set("ImageTrustCache", V::OfData(assets->at("Image.dmg.trustcache")));
        }
        if (!successful(s.request(mount), "Complete", error)) {
            return std::nullopt;
        }
        auto result = inspect(s, catalog.kind, error);
        if (!result) {
            return std::nullopt;
        }
        if (result->state != MountState::AlreadyMounted) {
            error = "DDI mount completed without a verifiable mounted image";
            return std::nullopt;
        }
        if (!result->signature.empty() && result->signature != signature) {
            error = "Mounted DDI signature differs from the uploaded image";
            return std::nullopt;
        }
        result->state = MountState::Mounted;
        if (!result->signature.empty()) {
            result->signature_matches = true;
        }
        return result;
    }
    catch (const std::exception& e) {
        error = safe_text(e.what());
        return std::nullopt;
    }
}
} // namespace detail

std::optional<MountResult> mount_image(
    PlistService& service,
    const DownloadResult& download,
    const DownloadTarget& target,
    uint64_t ecid,
    const MountOptions& options,
    std::string& error)
{
    const detail::Catalog* catalog = nullptr;
    if (download.kind == Kind::Personalized) {
        catalog = &detail::personalized_catalog();
    }
    else if (download.kind == Kind::Cryptex) {
        catalog = &detail::cryptex_catalog();
    }
    else {
        auto version = parse_product_version(target.system_version, error);
        if (version) {
            catalog = detail::classic_catalog(*version);
        }
    }
    if (!catalog) {
        error = "No pinned DDI catalog matches this installation target";
        return std::nullopt;
    }
    return detail::mount_image_with_catalog(service, download, target, ecid, options, *catalog, error);
}
} // namespace maa::ios::ddi
