#pragma once

#include "wifi/PairRecord.h"

namespace maa::ios::wifi
{
inline bool validate_credential(const PairRecord& record, std::string& error)
{
    if (!record.complete() || !record.has_peer_identity() || (!record.peer_alt_irk.empty() && record.peer_alt_irk.size() != 16)) {
        error = "Pairing credential must contain complete host and device identities";
        return false;
    }
    if (record.udid.size() > 1024 || record.host_identifier.size() > 1024 || record.peer_identifier.size() > kMaxPeerIdentifier
        || record.udid.find_first_of("\r\n") != std::string::npos || record.host_identifier.find_first_of("\r\n") != std::string::npos
        || record.peer_identifier.find_first_of("\r\n") != std::string::npos || record.udid.find('\0') != std::string::npos
        || record.host_identifier.find('\0') != std::string::npos || record.peer_identifier.find('\0') != std::string::npos) {
        error = "Invalid iOS pairing identity";
        return false;
    }
    return true;
}

inline std::optional<PairRecord> parse_credential(std::string_view value, std::string& error)
{
    if (value.size() > 32768) {
        error = "Invalid pairing credential";
        return std::nullopt;
    }
    auto record = parse_record(value, error);
    if (!record || !validate_credential(*record, error)) {
        return std::nullopt;
    }
    return record;
}

}
