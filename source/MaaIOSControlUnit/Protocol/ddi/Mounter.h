#pragma once

#include "ddi/Download.h"
#include "plist/Plist.h"
#include <vector>

namespace maa::ios::ddi
{

// The adapter owns framing, TLS/RSD and the live connection. Each operation
// must enforce the supplied remaining timeout, including reconnect/handshake.
struct PlistService
{
    std::function<std::optional<plist::Value>(const plist::Value&, std::chrono::milliseconds, std::string&)> request;
    std::function<std::optional<plist::Value>(std::chrono::milliseconds, std::string&)> receive;
    std::function<bool(const uint8_t*, size_t, std::chrono::milliseconds, std::string&)> send_bytes;
    std::function<bool(std::chrono::milliseconds, std::string&)> reconnect;
};

struct MountOptions
{
    std::chrono::seconds timeout { 300 };
    std::function<bool()> cancelled;
    std::function<void(uint64_t, uint64_t)> progress;
    bool replace_existing = false;
    // Returns an Apple-signed ApImg4Ticket. Identifiers and nonce are obtained
    // from this connection; the callback must also enforce remaining timeout.
    std::function<std::optional<std::vector<
        uint8_t>>(const plist::Value&, const plist::Value&, uint64_t, const std::vector<uint8_t>&, std::chrono::milliseconds, std::string&)>
        personalize;
};

enum class MountState
{
    NotMounted,
    AlreadyMounted,
    Mounted
};

struct MountResult
{
    MountState state = MountState::NotMounted;
    std::string image_type;
    std::string verification; // LookupImage signature or CopyDevices entry.
    std::vector<uint8_t> signature;
    // Empty means an existing image was observed without comparing our ticket.
    std::optional<bool> signature_matches;
};

// Cryptex DDIs are also visible as Personalized to mobile_image_mounter.
std::optional<MountResult> inspect_mounted(PlistService&, Kind, const MountOptions&, std::string& error);
// For Classic images, compares the mounted signature with the target's pinned
// catalog digest without downloading an image or accessing a local cache.
std::optional<MountResult> inspect_mounted(PlistService&, const DownloadTarget&, const MountOptions&, std::string& error);
// Explicit replacement only; never rolls a nonce. Verifies the image is gone.
bool unmount_current(PlistService&, Kind, const MountOptions&, std::string& error);
// Classic/Personalized only. Validates pinned cache, prepares signature/ticket,
// sends image and checks mounted state. AlreadyMounted is not an installation.
// A different or unverifiable Classic signature fails unless replacement is explicit.
std::optional<MountResult>
    mount_image(PlistService&, const DownloadResult&, const DownloadTarget&, uint64_t ecid, const MountOptions&, std::string& error);

namespace detail
{
struct Catalog;
// Offline protocol-test seam, never used with externally supplied catalogs.
std::optional<MountResult> mount_image_with_catalog(
    PlistService&,
    const DownloadResult&,
    const DownloadTarget&,
    uint64_t ecid,
    const MountOptions&,
    const Catalog&,
    std::string& error);
}
} // namespace maa::ios::ddi
