#pragma once

#include "wifi/PairRecord.h"
#include <nlohmann/json.hpp>
#include <set>

namespace maa::ios::wifi
{
inline bool parse_credentials(const nlohmann::json& options, std::vector<PairRecord>& records, std::string& error)
{
    const auto found = options.find("pairing_records");
    if (found == options.end()) {
        return true;
    }
    if (!found->is_array() || found->size() > 256) {
        error = "pairing_records must be an array of at most 256 opaque credentials";
        return false;
    }
    std::set<std::string> identities;
    for (const auto& value : *found) {
        if (!value.is_string() || value.get_ref<const std::string&>().size() > 16384) {
            error = "Invalid pairing credential";
            return false;
        }
        std::string detail;
        auto record = parse_record(value.get_ref<const std::string&>(), detail);
        if (!record || !record->complete() || !record->has_peer_identity() || !identities.insert(record->udid).second) {
            error = "Pairing credentials must be complete and have unique device identities";
            return false;
        }
        records.push_back(std::move(*record));
    }
    return true;
}
}
