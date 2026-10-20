#pragma once

#include "MaaFramework/MaaDef.h"
#include "wifi/Credentials.h"

namespace maa::ios::wifi
{
inline std::optional<PairRecord> copy_pairing(const MaaIOSWiFiPairing& value, std::string& error)
{
    if (!value.udid || !value.host_identifier || !value.peer_identifier) {
        error = "Incomplete iOS Wi-Fi pairing credentials";
        return std::nullopt;
    }
    PairRecord record;
    record.udid = value.udid;
    record.host_identifier = value.host_identifier;
    record.peer_identifier = value.peer_identifier;
    record.host_private_key.assign(value.host_private_key, value.host_private_key + 32);
    record.host_public_key.assign(value.host_public_key, value.host_public_key + 32);
    record.peer_public_key.assign(value.peer_public_key, value.peer_public_key + 32);
    if (value.has_peer_alt_irk) {
        record.peer_alt_irk.assign(value.peer_alt_irk, value.peer_alt_irk + 16);
    }
    if (!validate_credential(record, error)) {
        return std::nullopt;
    }
    return record;
}

}
