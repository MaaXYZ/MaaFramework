#pragma once

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "wifi/Crypto.h"

namespace maa::ios::wifi
{

/// Product display model, shared by USB INFO and the wireless advertisement,
/// handshake and INFO defaults. It is separate from the signed peer identity.
std::string_view host_model();

enum class HostMetadataSource
{
    MachineUuid,
    SystemInstance,
    HostIdentifier
};

struct HostMetadata
{
    std::string model;
    std::string serial_number;
    HostMetadataSource source = HostMetadataSource::HostIdentifier;
};

/// MAAIOS- plus 16 uppercase hex digits from an application-specific SHA-256.
/// Uses a readable machine UUID, then a system-instance ID (Linux machine-id or
/// Windows MachineGuid), then the exact existing host_identifier. Raw platform
/// identifiers and hardware serials are never published or persisted.
/// This is a display pseudonym, not a guarantee of unique hardware or trust.
/// Does not change pairing identifiers, keys, record format or existing peers.
std::optional<HostMetadata> host_metadata(std::string_view host_identifier, std::string& err);

namespace detail
{
using MachineUuid = std::array<uint8_t, 16>;

/// Canonical UUID bytes; accepts hex text with or without UUID separators and
/// surrounding ASCII whitespace. Rejects malformed, all-zero and all-FF UUIDs.
std::optional<MachineUuid> parse_machine_uuid(std::string_view text);

/// Parse the complete Windows RSMB buffer, including its eight-byte header.
/// SMBIOS >=2.6 uses little-endian first 4/2/2-byte UUID fields; older versions
/// use the stored byte order. Requires a complete type-1 structure/string area.
std::optional<MachineUuid> smbios_machine_uuid(std::span<const uint8_t> raw);

/// Pure derivation seam for known UUID/fallback vectors; no platform I/O.
std::optional<HostMetadata> host_metadata_for_uuid(
    std::string_view host_identifier,
    const std::optional<MachineUuid>& uuid,
    std::string& err,
    HostMetadataSource source = HostMetadataSource::MachineUuid);
} // namespace detail
} // namespace maa::ios::wifi
