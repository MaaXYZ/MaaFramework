#include "ddi/Mounter.h"
#include "ddi/DownloadInternal.h"
#include "i18n/Translation.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <openssl/evp.h>
#include <stdexcept>

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
    Clock::time_point deadline;
    bool copy_unsupported = false;

    Session(PlistService& s, const MountOptions& o)
        : service(s)
        , options(o)
    {
        if (o.timeout <= std::chrono::seconds::zero() || o.timeout > std::chrono::hours(24)) {
            throw std::runtime_error(MAA_IOS_TR("DDI install timeout must be positive and at most 24 hours"));
        }
        deadline = Clock::now() + o.timeout;
    }

    std::chrono::milliseconds remaining() const
    {
        if (options.cancelled && options.cancelled()) {
            throw std::runtime_error(MAA_IOS_TR("DDI install cancelled"));
        }
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now());
        if (left <= std::chrono::milliseconds::zero()) {
            throw std::runtime_error(MAA_IOS_TR("DDI install timed out"));
        }
        return left;
    }

    V check_reply(std::optional<V> reply, std::string& error)
    {
        remaining();
        if (!reply) {
            throw std::runtime_error(error.empty() ? MAA_IOS_TR("DDI service disconnected without a reply") : safe_text(error));
        }
        size_t budget = 2 * 1024 * 1024, nodes = 0;
        if (!reply->is_dict() || !bounded_value(*reply, 0, budget, nodes)) {
            throw std::runtime_error(MAA_IOS_TR("Invalid or oversized DDI service reply"));
        }
        return std::move(*reply);
    }

    V request(const V& request)
    {
        if (!service.request) {
            throw std::runtime_error(MAA_IOS_TR("DDI service request callback is missing"));
        }
        std::string error;
        return check_reply(service.request(request, remaining(), error), error);
    }

    void reconnect()
    {
        if (!service.reconnect) {
            throw std::runtime_error(MAA_IOS_TR("DDI service reconnect callback is missing"));
        }
        std::string error;
        if (!service.reconnect(remaining(), error)) {
            throw std::runtime_error(error.empty() ? MAA_IOS_TR("Cannot reconnect DDI service") : safe_text(error));
        }
        remaining();
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
        return MAA_IOS_TR("Invalid DDI service error field");
    }
    std::string text = safe_text(e->string);
    if (auto* detail = reply.find("DetailedError"); detail && detail->is_string()) {
        text += ": " + safe_text(detail->string);
    }
    return text;
}

void successful(const V& reply, std::string_view status)
{
    auto error = device_error(reply);
    if (!error.empty()) {
        throw std::runtime_error(error);
    }
    auto* value = reply.find("Status");
    if (!value || !value->is_string() || value->string != status) {
        throw std::runtime_error(std::string(MAA_IOS_TR("Unexpected DDI service status; expected ")) + std::string(status));
    }
}

MountResult inspect(Session& s, Kind kind)
{
    MountResult result;
    result.image_type = image_type(kind);
    auto request = command("LookupImage");
    request.set("ImageType", V::Str(result.image_type));
    auto reply = s.request(request);
    auto error = device_error(reply);
    if (!error.empty()) {
        throw std::runtime_error(error);
    }
    auto* present = reply.find("ImagePresent");
    if (present && !present->is_bool()) {
        throw std::runtime_error(MAA_IOS_TR("Invalid DDI ImagePresent field"));
    }
    auto* signature = reply.find("ImageSignature");
    if (signature && signature->is_array()) {
        if (signature->array.size() > 128) {
            throw std::runtime_error(MAA_IOS_TR("Too many mounted DDI signatures"));
        }
        for (const auto& entry : signature->array) {
            if (entry.kind != plist::Kind::Data || entry.data.size() > max_ticket) {
                throw std::runtime_error(MAA_IOS_TR("Invalid mounted DDI signature"));
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
        throw std::runtime_error(MAA_IOS_TR("Invalid mounted DDI signature"));
    }
    if (signature && !signature->data.empty()) {
        if (present && !present->boolean) {
            throw std::runtime_error(MAA_IOS_TR("Contradictory DDI lookup reply"));
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
        auto* entries = devices.find("EntryList");
        auto* e = devices.find("Error");
        bool unknown = e && e->is_string() && (e->string == "UnknownCommand" || e->string == "UnsupportedCommand");
        if (unknown) {
            s.copy_unsupported = true;
            s.reconnect();
        }
        else {
            auto detail = device_error(devices);
            if (!detail.empty()) {
                throw std::runtime_error(detail);
            }
            if (!entries || !entries->is_array() || entries->array.size() > 1024) {
                throw std::runtime_error(MAA_IOS_TR("Invalid DDI CopyDevices reply"));
            }
            for (const auto& entry : entries->array) {
                if (!entry.is_dict()) {
                    throw std::runtime_error(MAA_IOS_TR("Invalid DDI mounted image entry"));
                }
                auto* type = entry.find("DiskImageType");
                if (type && type->is_string() && type->string == result.image_type) {
                    // CopyDevices can describe attached disk images. Their
                    // type alone does not establish a mounted developer image.
                    const auto* mounted = entry.find("IsMounted");
                    if (!mounted || !mounted->is_bool()) {
                        throw std::runtime_error(MAA_IOS_TR("Invalid DDI mounted image entry"));
                    }
                    if (!mounted->boolean) {
                        continue;
                    }
                    const auto* path = entry.find("MountPath");
                    const std::string_view expected = kind == Kind::Classic ? "/Developer" : "/System/Developer";
                    if (!path || !path->is_string() || path->string != expected) {
                        throw std::runtime_error(MAA_IOS_TR("Invalid DDI mounted image entry"));
                    }
                    result.state = MountState::AlreadyMounted;
                    result.verification = "CopyDevices entry";
                    return result;
                }
            }
        }
    }
    if (present && present->boolean) {
        throw std::runtime_error(MAA_IOS_TR("DDI lookup claims presence without verifiable mounted image"));
    }
    return result;
}

void developer_mode(Session& s, uint32_t major)
{
    // Pre-iOS16 has no Developer Mode; probing unsupported legacy commands can
    // close the service. Modern devices must provide a definite enabled state.
    if (major < 16) {
        return;
    }
    auto reply = s.request(command("QueryDeveloperModeStatus"));
    auto error = device_error(reply);
    if (!error.empty()) {
        throw std::runtime_error(error);
    }
    const auto* enabled = reply.find("DeveloperModeStatus");
    if (!enabled || !enabled->is_bool()) {
        throw std::runtime_error(MAA_IOS_TR("Cannot verify device Developer Mode"));
    }
    if (!enabled->boolean) {
        throw std::runtime_error(MAA_IOS_TR("Enable Developer Mode on the device before installing DDI"));
    }
}

void unmount(Session& s, Kind kind)
{
    auto request = command("UnmountImage");
    request.set("MountPath", V::Str(kind == Kind::Classic ? "/Developer" : "/System/Developer"));
    auto reply = s.request(request);
    successful(reply, "Complete");
    if (inspect(s, kind).state != MountState::NotMounted) {
        throw std::runtime_error(MAA_IOS_TR("DDI remains mounted after unmount request"));
    }
}

std::vector<uint8_t> hash_bytes(const std::vector<uint8_t>& data, const EVP_MD* algorithm, bool git)
{
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx || EVP_DigestInit_ex(ctx.get(), algorithm, nullptr) != 1) {
        throw std::runtime_error(MAA_IOS_TR("Cannot hash DDI asset"));
    }
    if (git) {
        auto prefix = "blob " + std::to_string(data.size());
        prefix.push_back('\0');
        if (EVP_DigestUpdate(ctx.get(), prefix.data(), prefix.size()) != 1) {
            throw std::runtime_error(MAA_IOS_TR("Cannot hash DDI asset"));
        }
    }
    std::vector<uint8_t> output(EVP_MAX_MD_SIZE);
    unsigned size = 0;
    if (EVP_DigestUpdate(ctx.get(), data.data(), data.size()) != 1 || EVP_DigestFinal_ex(ctx.get(), output.data(), &size) != 1) {
        throw std::runtime_error(MAA_IOS_TR("Cannot hash DDI asset"));
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

std::map<std::string, std::vector<uint8_t>> load_assets(Session& s, const DownloadResult& download, const detail::Catalog& catalog)
{
    std::map<std::string, std::vector<uint8_t>> assets;
    for (const auto& asset : catalog.assets) {
        s.remaining();
        if (asset.size > max_file) {
            throw std::runtime_error(MAA_IOS_TR("DDI asset exceeds install size limit"));
        }
        std::ifstream file(download.directory / asset.name, std::ios::binary);
        std::vector<uint8_t> bytes(static_cast<size_t>(asset.size));
        if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())) || file.peek() != EOF) {
            throw std::runtime_error(MAA_IOS_TR("Cannot read complete DDI install asset"));
        }
        // Hash the actual snapshot sent to the device, not only the earlier
        // on-disk validation. Mutation after validation cannot alter upload.
        if ((!asset.sha256.empty() && hex(hash_bytes(bytes, EVP_sha256(), false)) != asset.sha256)
            || (!asset.git_blob_sha1.empty() && hex(hash_bytes(bytes, EVP_sha1(), true)) != asset.git_blob_sha1)) {
            throw std::runtime_error(MAA_IOS_TR("DDI install asset changed after cache validation"));
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
        Session session(service, options);
        return inspect(session, kind);
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
        mounted->signature_matches =
            mounted->signature.size() == asset->size
            && (asset->sha256.empty() || hex(hash_bytes(mounted->signature, EVP_sha256(), false)) == asset->sha256)
            && (asset->git_blob_sha1.empty() || hex(hash_bytes(mounted->signature, EVP_sha1(), true)) == asset->git_blob_sha1);
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
        Session session(service, options);
        unmount(session, kind);
        return true;
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
        Session s(service, options);
        s.remaining();
        if (catalog.kind == Kind::Cryptex) {
            throw std::runtime_error(MAA_IOS_TR("Cryptex DDI requires the cryptexd installer"));
        }
        if (download.kind != catalog.kind || download.build != catalog.build) {
            throw std::runtime_error(MAA_IOS_TR("DDI install result does not match its catalog"));
        }
        auto version = parse_product_version(target.system_version, error);
        auto selected = version ? select_for_version(*version, error) : std::nullopt;
        if (!selected || selected->kind != catalog.kind) {
            throw std::runtime_error(MAA_IOS_TR("DDI install category does not match device OS"));
        }
        if (!validate(download.directory, catalog, error, &target)) {
            throw std::runtime_error(error);
        }
        auto assets = load_assets(s, download, catalog);
        auto existing = inspect(s, catalog.kind);
        if (existing.state == MountState::AlreadyMounted && !options.replace_existing) {
            if (catalog.kind == Kind::Classic) {
                existing.signature_matches = existing.signature == assets.at("DeveloperDiskImage.dmg.signature");
            }
            return existing;
        }
        developer_mode(s, version->major);
        std::vector<uint8_t> signature;
        const auto& image = assets.at(catalog.kind == Kind::Classic ? "DeveloperDiskImage.dmg" : "Image.dmg");
        if (catalog.kind == Kind::Classic) {
            signature = assets.at("DeveloperDiskImage.dmg.signature");
        }
        else {
            if (!options.personalize || !ecid) {
                throw std::runtime_error(MAA_IOS_TR("DDI personalization requires a ticket callback and device ECID"));
            }
            auto identifiers_reply = s.request(command("QueryPersonalizationIdentifiers"));
            auto e = device_error(identifiers_reply);
            if (!e.empty()) {
                throw std::runtime_error(e);
            }
            auto* identifiers = identifiers_reply.find("PersonalizationIdentifiers");
            auto board = identifiers ? unsigned_number(identifiers->find("BoardId")) : std::nullopt;
            auto chip = identifiers ? unsigned_number(identifiers->find("ChipID")) : std::nullopt;
            if (!identifiers || !identifiers->is_dict() || !board || !chip) {
                throw std::runtime_error(MAA_IOS_TR("Invalid DDI personalization identifiers"));
            }
            if ((target.board_id && target.board_id != board) || (target.chip_id && target.chip_id != chip)) {
                throw std::runtime_error(MAA_IOS_TR("DDI device hardware identity changed"));
            }
            DownloadTarget confirmed = target;
            confirmed.board_id = board;
            confirmed.chip_id = chip;
            if (!validate(download.directory, catalog, error, &confirmed)) {
                throw std::runtime_error(error);
            }
            auto nonce_request = command("QueryNonce");
            nonce_request.set("PersonalizedImageType", V::Str("DeveloperDiskImage"));
            auto nonce_reply = s.request(nonce_request);
            e = device_error(nonce_reply);
            if (!e.empty()) {
                throw std::runtime_error(e);
            }
            auto* nonce = nonce_reply.find("PersonalizationNonce");
            if (!nonce || nonce->kind != plist::Kind::Data || nonce->data.empty() || nonce->data.size() > 128) {
                throw std::runtime_error(MAA_IOS_TR("Invalid DDI personalization nonce"));
            }
            auto& manifest_bytes = assets.at("BuildManifest.plist");
            auto manifest =
                plist::parse(std::string_view(reinterpret_cast<const char*>(manifest_bytes.data()), manifest_bytes.size()), &error);
            if (!manifest) {
                throw std::runtime_error(MAA_IOS_TR("Cannot parse DDI install manifest"));
            }
            auto ticket = options.personalize(*manifest, *identifiers, ecid, nonce->data, s.remaining(), error);
            s.remaining();
            if (!ticket) {
                throw std::runtime_error(error.empty() ? MAA_IOS_TR("DDI personalization did not return a ticket") : safe_text(error));
            }
            signature = std::move(*ticket);
            // Recheck the live nonce after HTTP. A reboot/other installer must
            // not cause this connection to upload a ticket for an earlier nonce.
            auto current = s.request(nonce_request);
            e = device_error(current);
            if (!e.empty()) {
                throw std::runtime_error(e);
            }
            auto* current_nonce = current.find("PersonalizationNonce");
            if (!current_nonce || current_nonce->kind != plist::Kind::Data || current_nonce->data != nonce->data) {
                throw std::runtime_error(MAA_IOS_TR("DDI personalization nonce changed before upload"));
            }
        }
        if (signature.empty() || signature.size() > max_ticket) {
            throw std::runtime_error(MAA_IOS_TR("Invalid DDI install signature or ticket"));
        }
        if (existing.state == MountState::AlreadyMounted) {
            unmount(s, catalog.kind);
        }
        auto upload = command("ReceiveBytes");
        upload.set("ImageType", V::Str(image_type(catalog.kind)));
        upload.set("ImageSize", V::Int(static_cast<int64_t>(image.size())));
        upload.set("ImageSignature", V::OfData(signature));
        successful(s.request(upload), "ReceiveBytesAck");
        if (!service.send_bytes || !service.receive) {
            throw std::runtime_error(MAA_IOS_TR("DDI image transfer callbacks are missing"));
        }
        for (size_t offset = 0; offset < image.size();) {
            auto amount = std::min<size_t>(64 * 1024, image.size() - offset);
            if (!service.send_bytes(image.data() + offset, amount, s.remaining(), error)) {
                throw std::runtime_error(error.empty() ? MAA_IOS_TR("DDI image upload failed") : safe_text(error));
            }
            offset += amount;
            s.remaining();
            if (options.progress) {
                options.progress(offset, image.size());
            }
        }
        successful(s.check_reply(service.receive(s.remaining(), error), error), "Complete");
        // A concurrent installer must not be mistaken for our own mount.
        if (inspect(s, catalog.kind).state != MountState::NotMounted) {
            throw std::runtime_error(MAA_IOS_TR("Another DDI became mounted during upload"));
        }
        auto mount = command("MountImage");
        mount.set("ImageType", V::Str(image_type(catalog.kind)));
        mount.set("ImageSignature", V::OfData(signature));
        if (catalog.kind == Kind::Personalized) {
            mount.set("ImageTrustCache", V::OfData(assets.at("Image.dmg.trustcache")));
        }
        successful(s.request(mount), "Complete");
        auto result = inspect(s, catalog.kind);
        if (result.state != MountState::AlreadyMounted) {
            throw std::runtime_error(MAA_IOS_TR("DDI mount completed without a verifiable mounted image"));
        }
        if (!result.signature.empty() && result.signature != signature) {
            throw std::runtime_error(MAA_IOS_TR("Mounted DDI signature differs from the uploaded image"));
        }
        result.state = MountState::Mounted;
        if (!result.signature.empty()) {
            result.signature_matches = true;
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
        error = MAA_IOS_TR("No pinned DDI catalog matches this installation target");
        return std::nullopt;
    }
    return detail::mount_image_with_catalog(service, download, target, ecid, options, *catalog, error);
}
} // namespace maa::ios::ddi
