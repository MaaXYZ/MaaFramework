#include "ddi/Cryptex.h"
#include "ddi/DownloadInternal.h"
#include "ddi/Mounter.h"
#include "ddi/Tss.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
using namespace maa::ios;
using V = plist::Value;

void require(bool value, const char* message)
{
    if (!value) {
        throw std::runtime_error(message);
    }
}

// iOS 11.4 signature, pinned by classic_catalog's Git blob digest.
const std::vector<uint8_t> classic_signature {
    0x72, 0xc0, 0xa3, 0x4f, 0x3d, 0xfb, 0x8e, 0xcf, 0x5c, 0xc6, 0x49, 0xb9, 0x3d, 0x8e, 0x51, 0xe5, 0x49, 0xa6, 0x9e, 0x30, 0xc7, 0x63,
    0x8a, 0xa9, 0xbe, 0xbf, 0x4a, 0xce, 0x84, 0x5f, 0xc5, 0xb6, 0x2d, 0x2e, 0x88, 0x98, 0xa4, 0xee, 0xab, 0x78, 0x7c, 0xd7, 0xff, 0xaa,
    0x7c, 0xea, 0x90, 0x93, 0x14, 0xd9, 0x62, 0x0f, 0xbb, 0xda, 0xf6, 0x5d, 0x9d, 0x7a, 0xfd, 0xac, 0xf0, 0xfe, 0x87, 0x56, 0x31, 0x2c,
    0xb6, 0x96, 0xdf, 0x21, 0x7c, 0x65, 0x06, 0x39, 0xf1, 0xbd, 0x57, 0x58, 0xa1, 0x3b, 0x55, 0xda, 0x36, 0xac, 0x48, 0xd7, 0xfc, 0x53,
    0x9e, 0x90, 0x9e, 0x2b, 0x45, 0x5c, 0x49, 0x54, 0x95, 0xbc, 0x29, 0x3c, 0xc1, 0xfc, 0xef, 0x7e, 0x0d, 0xf5, 0xbe, 0x05, 0x15, 0x61,
    0xb0, 0x9d, 0x65, 0x03, 0x2d, 0x6b, 0xdd, 0x51, 0x32, 0x70, 0xfe, 0x59, 0x42, 0x6e, 0xc8, 0x5e, 0x0c, 0x9e,
};

struct ImageMounter
{
    std::vector<uint8_t> signature = classic_signature;
    bool mounted = true;
    size_t uploaded = 0;
    std::vector<std::string> commands;

    ddi::PlistService service()
    {
        ddi::PlistService result;
        result.request = [this](const V& request, auto, std::string&) -> std::optional<V> {
            const auto command = request.find("Command")->as_string_or();
            commands.push_back(command);
            auto reply = V::Dict();
            if (command == "LookupImage") {
                reply.set("ImagePresent", V::Bool(mounted));
                reply.set("ImageSignature", V::OfData(mounted ? signature : std::vector<uint8_t> { }));
            }
            else if (command == "CopyDevices") {
                auto entries = V::Array();
                if (mounted) {
                    auto entry = V::Dict();
                    entry.set("DiskImageType", V::Str("Developer"));
                    entry.set("IsMounted", V::Bool(true));
                    entry.set("MountPath", V::Str("/Developer"));
                    entries.push(std::move(entry));
                }
                reply.set("EntryList", std::move(entries));
            }
            else if (command == "QueryDeveloperModeStatus") {
                reply.set("DeveloperModeStatus", V::Bool(true));
            }
            else if (command == "UnmountImage") {
                mounted = false;
                reply.set("Status", V::Str("Complete"));
            }
            else if (command == "ReceiveBytes") {
                reply.set("Status", V::Str("ReceiveBytesAck"));
            }
            else if (command == "MountImage") {
                mounted = true;
                signature = request.find("ImageSignature")->data;
                reply.set("Status", V::Str("Complete"));
            }
            else {
                throw std::runtime_error("Unexpected image-mounter command");
            }
            return reply;
        };
        result.send_bytes = [this](const uint8_t* bytes, size_t size, auto, std::string&) {
            require(size == 1 && bytes[0] == 'x', "only the selected image is uploaded");
            uploaded += size;
            return true;
        };
        result.receive = [](auto, std::string&) -> std::optional<V> {
            auto reply = V::Dict();
            reply.set("Status", V::Str("Complete"));
            return reply;
        };
        return result;
    }
};

void test_classic_reuse_without_download()
{
    ImageMounter mock;
    auto service = mock.service();
    ddi::DownloadTarget target;
    target.system_version = "11.4.1";
    ddi::MountOptions options;
    std::string error;
    auto result = ddi::inspect_mounted(service, target, options, error);
    require(
        result && result->state == ddi::MountState::AlreadyMounted && result->signature_matches == true && error.empty(),
        "pinned Classic signature is reusable without cache or download");
    require(mock.commands == std::vector<std::string> { "LookupImage" }, "reuse only queries the mounted signature");

    mock.signature[0] ^= 1;
    result = ddi::inspect_mounted(service, target, options, error);
    require(result && result->signature_matches == false, "different Classic signature cannot be reused");
    mock.signature.clear();
    result = ddi::inspect_mounted(service, target, options, error);
    require(
        result && result->state == ddi::MountState::AlreadyMounted && result->signature_matches == false,
        "mounted state alone does not establish Classic compatibility");
    mock.mounted = false;
    result = ddi::inspect_mounted(service, target, options, error);
    require(
        result && result->state == ddi::MountState::NotMounted && !result->signature_matches,
        "absent Classic image continues to preparation");
    mock.mounted = true;
    target.system_version = "10.0";
    result = ddi::inspect_mounted(service, target, options, error);
    require(!result && !error.empty(), "unsupported Classic catalog cannot establish compatibility");
}

void test_existing_classic_signature_mismatch()
{
    const ddi::detail::Catalog catalog { "16.0",
                                         "https://example.invalid/ddi",
                                         { { "DeveloperDiskImage.dmg", 1, "", "c1b0730e0133447badcfd47fd144e254807b06e1" },
                                           { "DeveloperDiskImage.dmg.signature", 1, "", "e25f1814e51579d5f55c0f1fe0135ddb28a47f4a" } },
                                         ddi::Kind::Classic };
    std::string error;
    auto download = ddi::detail::download(
        { },
        catalog,
        [](const std::string& url, auto, auto, const auto&, const auto& sink, std::string&) {
            const char value = url.ends_with(".signature") ? 'y' : 'x';
            return sink(&value, 1);
        },
        error);
    require(download.has_value(), "synthetic Classic cache is valid");
    ddi::DownloadTarget target;
    target.system_version = "16.0";
    ddi::MountOptions options;
    ImageMounter mock;
    mock.signature = { 'y' };
    auto service = mock.service();
    auto result = ddi::detail::mount_image_with_catalog(service, *download, target, 0, options, catalog, error);
    require(
        result && result->state == ddi::MountState::AlreadyMounted && result->signature_matches == true,
        "matching mounted Classic image is reused");
    require(mock.uploaded == 0 && mock.commands == std::vector<std::string> { "LookupImage" }, "matching reuse performs no device writes");

    mock.signature = { 'z' };
    mock.commands.clear();
    result = ddi::detail::mount_image_with_catalog(service, *download, target, 0, options, catalog, error);
    require(!result && !error.empty(), "mismatched mounted Classic image must fail instead of reporting success");
    require(
        mock.uploaded == 0 && mock.commands == std::vector<std::string> { "LookupImage" },
        "signature mismatch cannot implicitly replace the mounted image");

    mock.signature.clear();
    result = ddi::detail::mount_image_with_catalog(service, *download, target, 0, options, catalog, error);
    require(!result && !error.empty(), "mounted Classic image without a verifiable signature is rejected");

    mock.signature = { 'z' };
    options.replace_existing = true;
    const auto request = service.request;
    for (const auto failure : { "LookupImage", "UnmountImage", "ReceiveBytes" }) {
        mock.commands.clear();
        mock.mounted = true;
        mock.uploaded = 0;
        service.request = [&, failure](const V& value, auto timeout, std::string& message) -> std::optional<V> {
            if (value.find("Command")->as_string_or() == failure) {
                mock.commands.emplace_back(failure);
                message = "device disconnected";
                return std::nullopt;
            }
            return request(value, timeout, message);
        };
        result = ddi::detail::mount_image_with_catalog(service, *download, target, 0, options, catalog, error);
        require(!result && error == "device disconnected", "device error is propagated");
        require(mock.commands.back() == failure && mock.uploaded == 0, "failed request stops subsequent device operations");
        require(mock.mounted == (std::string_view(failure) != "ReceiveBytes"), "failure cannot trigger an extra unmount");
    }
    service.request = request;
    mock.mounted = true;
    mock.commands.clear();
    result = ddi::detail::mount_image_with_catalog(service, *download, target, 0, options, catalog, error);
    require(
        result && result->state == ddi::MountState::Mounted && result->signature_matches == true && mock.uploaded == 1,
        "explicit replacement uploads and verifies the selected image");
    require(std::ranges::find(mock.commands, "UnmountImage") != mock.commands.end(), "replacement explicitly unmounts the previous image");
}

void test_inspection_failures()
{
    ImageMounter mock;
    auto service = mock.service();
    ddi::DownloadTarget target;
    target.system_version = "11.4.1";
    ddi::MountOptions options;
    std::string error;
    options.timeout = std::chrono::seconds::zero();
    require(!ddi::inspect_mounted(service, target, options, error) && !error.empty(), "invalid timeout fails before I/O");
    options.timeout = std::chrono::seconds(30);
    options.cancelled = [] {
        return true;
    };
    require(
        !ddi::inspect_mounted(service, target, options, error) && error.find("cancelled") != std::string::npos,
        "cancelled inspection fails");
    require(mock.commands.empty(), "invalid or cancelled inspection makes no device request");
    options.cancelled = { };
    service.request = [](const V&, auto, std::string&) -> std::optional<V> {
        return V::Array();
    };
    require(
        !ddi::inspect_mounted(service, target, options, error) && error.find("reply") != std::string::npos,
        "malformed reply is rejected");
    service.request = [](const V&, auto, std::string&) -> std::optional<V> {
        throw std::runtime_error("callback failure");
    };
    require(
        !ddi::inspect_mounted(service, target, options, error) && error == "callback failure",
        "callback exception is contained at the boundary");
    service.request = { };
    require(
        !ddi::inspect_mounted(service, target, options, error) && error.find("callback") != std::string::npos,
        "missing callback is rejected");
}

void test_signing_metadata_failures()
{
    auto manifest = V::Dict();
    auto identities = V::Array();
    auto identity = V::Dict();
    identity.set("ApBoardID", V::Int(1));
    identity.set("ApChipID", V::Int(2));
    auto info = V::Dict();
    info.set("Variant", V::Str("Developer PDI"));
    identity.set("Info", info);
    auto components = V::Dict();
    auto component = V::Dict();
    component.set("Info", V::Dict());
    component.set("Trusted", V::Bool(true));
    component.set("Digest", V::OfData({ 1, 2 }));
    components.set("PersonalizedDMG", component);
    components.set("LoadableTrustCache", component);
    identity.set("Manifest", components);
    identities.push(identity);
    manifest.set("BuildIdentities", identities);
    auto identifiers = V::Dict();
    identifiers.set("BoardId", V::Int(1));
    identifiers.set("ChipID", V::Int(2));
    std::string error;
    auto request = ddi::build_personalized_tss_request(manifest, identifiers, 3, { 4 }, error);
    require(request && error.empty() && request->find("PersonalizedDMG"), "valid metadata produces a signing request");
    require(!ddi::build_personalized_tss_request(manifest, identifiers, 3, { }, error) && !error.empty(), "empty nonce is rejected");
    auto rules = V::Dict();
    rules.set("RestoreRequestRules", V::Str("invalid"));
    component.set("Info", rules);
    components.set("LoadableTrustCache", component);
    identity.set("Manifest", components);
    identities.array[0] = identity;
    manifest.set("BuildIdentities", identities);
    require(
        !ddi::build_personalized_tss_request(manifest, identifiers, 3, { 4 }, error) && error.find("rules") != std::string::npos,
        "malformed signing rules cannot produce a request");

    info.set("Variant", V::Str("Developer Disk Image Cryptex"));
    identity.set("Info", info);
    identity.set("Cryptex1,NonceDomain", V::Int(4));
    identity.set("Cryptex1,UseProductClass", V::Bool(false));
    identity.set("Cryptex1,SubType", V::Int(1));
    identity.set("Cryptex1,Version", V::Str("1"));
    identity.set("Cryptex1,PreauthorizationVersion", V::Str("1"));
    identities.array[0] = identity;
    manifest.set("BuildIdentities", identities);
    auto bundle = V::Dict();
    bundle.set("CFBundleIdentifier", V::Str("com.apple.MobileAsset.DDI"));
    bundle.set("CFBundleVersion", V::Str("1"));
    bundle.set("RequiredMountPath", V::Str("/System/Developer"));
    require(
        ddi::detail::parse_cryptex_metadata(manifest, bundle, error).has_value() && error.empty(),
        "valid cryptex metadata is accepted");
    bundle.set("RequiredMountPath", V::Int(1));
    require(!ddi::detail::parse_cryptex_metadata(manifest, bundle, error) && !error.empty(), "malformed cryptex properties are rejected");
}
} // namespace

int main()
{
    try {
        test_classic_reuse_without_download();
        test_existing_classic_signature_mismatch();
        test_inspection_failures();
        test_signing_metadata_failures();
        std::cout << "Classic DDI mounted-image checks passed\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
