#include "wifi/HostMetadata.h"


#include <algorithm>
#include <fstream>
#include <openssl/evp.h>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace maa::ios::wifi
{
namespace
{
bool valid_uuid(const detail::MachineUuid& uuid)
{
    return !std::all_of(uuid.begin(), uuid.end(), [](uint8_t b) { return b == 0; })
           && !std::all_of(uuid.begin(), uuid.end(), [](uint8_t b) { return b == 0xff; });
}

struct PlatformIdentity
{
    detail::MachineUuid uuid;
    HostMetadataSource source;
};

#ifdef __linux__
std::optional<detail::MachineUuid> read_uuid_file(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    std::array<char, 80> text { };
    file.read(text.data(), text.size());
    const auto size = file.gcount();
    if (size <= 0 || size == static_cast<std::streamsize>(text.size())) {
        return std::nullopt;
    }
    return detail::parse_machine_uuid(std::string_view(text.data(), static_cast<size_t>(size)));
}
#endif

std::optional<PlatformIdentity> platform_identity()
{
#ifdef __APPLE__
    const io_service_t platform = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
    if (!platform) {
        return std::nullopt;
    }
    const CFTypeRef value = IORegistryEntryCreateCFProperty(platform, CFSTR("IOPlatformUUID"), kCFAllocatorDefault, 0);
    IOObjectRelease(platform);
    if (!value) {
        return std::nullopt;
    }
    std::array<char, 80> text { };
    const bool ok = CFGetTypeID(value) == CFStringGetTypeID()
                    && CFStringGetCString(static_cast<CFStringRef>(value), text.data(), text.size(), kCFStringEncodingUTF8);
    CFRelease(value);
    const auto uuid = ok ? detail::parse_machine_uuid(text.data()) : std::nullopt;
    return uuid ? std::optional(PlatformIdentity { *uuid, HostMetadataSource::MachineUuid }) : std::nullopt;
#elif defined(_WIN32)
    constexpr DWORD provider = 0x52534d42; // 'RSMB', as specified by Win32.
    const UINT size = GetSystemFirmwareTable(provider, 0, nullptr, 0);
    if (size >= 8 && size <= 1024 * 1024) {
        Bytes raw(size);
        const UINT written = GetSystemFirmwareTable(provider, 0, raw.data(), size);
        if (written == size) {
            if (const auto uuid = detail::smbios_machine_uuid(raw)) {
                return PlatformIdentity { *uuid, HostMetadataSource::MachineUuid };
            }
        }
    }
    // A stable OS-instance fallback; copied Windows installations may share it.
    std::array<char, 80> text { };
    DWORD bytes = static_cast<DWORD>(text.size());
    const LSTATUS status = RegGetValueA(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Cryptography",
        "MachineGuid",
        RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
        nullptr,
        text.data(),
        &bytes);
    if (status != ERROR_SUCCESS || bytes < 2 || bytes > text.size() || text[bytes - 1] != '\0') {
        return std::nullopt;
    }
    const auto uuid = detail::parse_machine_uuid(std::string_view(text.data(), bytes - 1));
    return uuid ? std::optional(PlatformIdentity { *uuid, HostMetadataSource::SystemInstance }) : std::nullopt;
#elif defined(__linux__)
    // May be absent on ARM or unreadable by an ordinary user; no privilege change.
    if (const auto uuid = read_uuid_file("/sys/class/dmi/id/product_uuid")) {
        return PlatformIdentity { *uuid, HostMetadataSource::MachineUuid };
    }
    // Usually readable without privileges and stable for this system instance.
    // Cloned installations may share it; it is not a hardware-uniqueness claim.
    for (const char* path : { "/etc/machine-id", "/var/lib/dbus/machine-id" }) {
        if (const auto uuid = read_uuid_file(path)) {
            return PlatformIdentity { *uuid, HostMetadataSource::SystemInstance };
        }
    }
    return std::nullopt;
#else
    return std::nullopt;
#endif
}
} // namespace

std::string_view host_model()
{
#ifdef __APPLE__
    return "MaaFW-MacOS";
#elif defined(_WIN32)
    return "MaaFW-Windows";
#elif defined(__ANDROID__)
    return "MaaFW-Android";
#elif defined(__linux__)
    return "MaaFW-Linux";
#else
    return "MaaFW-Unknown";
#endif
}

std::optional<detail::MachineUuid> detail::parse_machine_uuid(std::string_view text)
{
    const auto space = [](char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    };
    while (!text.empty() && space(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && space(text.back())) {
        text.remove_suffix(1);
    }
    if (text.size() != 32 && text.size() != 36) {
        return std::nullopt;
    }
    const auto hex = [](char c) {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return -1;
    };
    MachineUuid out { };
    size_t digits = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text.size() == 36 && (i == 8 || i == 13 || i == 18 || i == 23)) {
            if (text[i] != '-') {
                return std::nullopt;
            }
            continue;
        }
        const int value = hex(text[i]);
        if (value < 0) {
            return std::nullopt;
        }
        out[digits / 2] |= static_cast<uint8_t>(value << (digits % 2 ? 0 : 4));
        ++digits;
    }
    return valid_uuid(out) ? std::optional(out) : std::nullopt;
}

std::optional<detail::MachineUuid> detail::smbios_machine_uuid(std::span<const uint8_t> raw)
{
    if (raw.size() < 8) {
        return std::nullopt;
    }
    const uint32_t size = uint32_t(raw[4]) | (uint32_t(raw[5]) << 8) | (uint32_t(raw[6]) << 16) | (uint32_t(raw[7]) << 24);
    if (size > raw.size() - 8) {
        return std::nullopt;
    }
    const bool little = raw[1] > 2 || (raw[1] == 2 && raw[2] >= 6);
    const auto table = raw.subspan(8, size);
    for (size_t at = 0; at < table.size();) {
        if (table.size() - at < 4) {
            return std::nullopt;
        }
        const uint8_t type = table[at];
        const size_t length = table[at + 1];
        if (length < 4 || length > table.size() - at) {
            return std::nullopt;
        }
        size_t end = at + length;
        while (end + 1 < table.size() && (table[end] != 0 || table[end + 1] != 0)) {
            ++end;
        }
        if (end + 1 >= table.size()) {
            return std::nullopt;
        }
        if (type == 1 && length >= 24) {
            MachineUuid uuid { };
            std::copy_n(table.begin() + at + 8, uuid.size(), uuid.begin());
            if (little) {
                std::reverse(uuid.begin(), uuid.begin() + 4);
                std::reverse(uuid.begin() + 4, uuid.begin() + 6);
                std::reverse(uuid.begin() + 6, uuid.begin() + 8);
            }
            return valid_uuid(uuid) ? std::optional(uuid) : std::nullopt;
        }
        if (type == 127) {
            break;
        }
        at = end + 2;
    }
    return std::nullopt;
}

std::optional<HostMetadata> detail::host_metadata_for_uuid(
    std::string_view host_identifier,
    const std::optional<MachineUuid>& uuid,
    std::string& err,
    HostMetadataSource source)
{
    err.clear();
    const bool platform = uuid && valid_uuid(*uuid) && source != HostMetadataSource::HostIdentifier;
    if (!platform && host_identifier.empty()) {
        err = "Host display metadata requires a platform identifier or host identifier";
        return std::nullopt;
    }
    Bytes input = bytes_of("MaaFramework.ios.host-display-serial.v1");
    input.push_back(0);
    const auto domain =
        bytes_of(platform ? (source == HostMetadataSource::SystemInstance ? "system-instance" : "machine-uuid") : "host-identity");
    input.insert(input.end(), domain.begin(), domain.end());
    input.push_back(0);
    if (platform) {
        input.insert(input.end(), uuid->begin(), uuid->end());
    }
    else {
        input.insert(input.end(), host_identifier.begin(), host_identifier.end());
    }
    std::array<uint8_t, 32> digest { };
    unsigned int length = 0;
    if (EVP_Digest(input.data(), input.size(), digest.data(), &length, EVP_sha256(), nullptr) != 1 || length != digest.size()) {
        err = "Cannot hash the host display serial number";
        return std::nullopt;
    }
    HostMetadata out { std::string(host_model()), "MaaFW-", platform ? source : HostMetadataSource::HostIdentifier };
    constexpr char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < 8; ++i) {
        out.serial_number += hex[digest[i] >> 4];
        out.serial_number += hex[digest[i] & 15];
    }
    return out;
}

std::optional<HostMetadata> host_metadata(std::string_view host_identifier, std::string& err)
{
    const auto platform = platform_identity();
    return detail::host_metadata_for_uuid(
        host_identifier,
        platform ? std::optional(platform->uuid) : std::nullopt,
        err,
        platform ? platform->source : HostMetadataSource::HostIdentifier);
}
} // namespace maa::ios::wifi
