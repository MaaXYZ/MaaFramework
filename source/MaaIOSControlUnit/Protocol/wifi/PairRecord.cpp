#include "wifi/PairRecord.h"
#include "i18n/Translation.h"

#include <set>
#include <sstream>

namespace maa::ios::wifi
{
namespace
{

constexpr char kHeader[] = "MaaIOS-pair-record 1";

std::string hex(const Bytes& data)
{
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (const uint8_t b : data) {
        out += kDigits[b >> 4];
        out += kDigits[b & 0xF];
    }
    return out;
}

std::optional<Bytes> unhex(std::string_view text, std::string& err)
{
    if (text.size() % 2 != 0) {
        err = MAA_IOS_TR("Hex length must be even");
        return std::nullopt;
    }
    auto nibble = [](char c) -> int {
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
    Bytes out;
    out.reserve(text.size() / 2);
    for (size_t i = 0; i < text.size(); i += 2) {
        const int hi = nibble(text[i]), lo = nibble(text[i + 1]);
        if (hi < 0 || lo < 0) {
            err = MAA_IOS_TR("Invalid hex character");
            return std::nullopt;
        }
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return out;
}

std::string_view trim(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
        s.remove_prefix(1);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) {
        s.remove_suffix(1);
    }
    return s;
}

} // namespace

std::string format_record(const PairRecord& record)
{
    std::ostringstream out;
    out << kHeader << '\n';
    out << "udid=" << record.udid << '\n';
    out << "host_identifier=" << record.host_identifier << '\n';
    out << "host_private_key=" << hex(record.host_private_key) << '\n';
    out << "host_public_key=" << hex(record.host_public_key) << '\n';
    if (!record.advertised_identifier.empty()) {
        out << "advertised_identifier=" << record.advertised_identifier << '\n';
    }
    if (!record.peer_identifier.empty()) {
        out << "peer_identifier=" << hex(record.peer_identifier) << '\n';
    }
    if (!record.peer_public_key.empty()) {
        out << "peer_public_key=" << hex(record.peer_public_key) << '\n';
    }
    if (!record.peer_alt_irk.empty()) {
        out << "peer_alt_irk=" << hex(record.peer_alt_irk) << '\n';
    }
    if (!record.remote_unlock_host_key.empty()) {
        out << "remote_unlock_host_key=" << record.remote_unlock_host_key << '\n';
    }
    return out.str();
}

std::optional<PairRecord> parse_record(std::string_view text, std::string& err)
{
    PairRecord record;
    bool first = true;
    bool seen_private = false;
    std::set<std::string_view> seen_fields;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) {
            nl = text.size();
        }
        const std::string_view line = trim(text.substr(pos, nl - pos));
        pos = nl + 1;
        if (line.empty()) {
            continue;
        }
        if (first) {
            first = false;
            if (line != kHeader) {
                err = MAA_IOS_TR("Invalid MaaIOS pairing record header");
                return std::nullopt;
            }
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string_view::npos) {
            err = MAA_IOS_TR("Record line is not key=value");
            return std::nullopt;
        }
        const std::string_view key = trim(line.substr(0, eq));
        const std::string_view value = trim(line.substr(eq + 1));
        const bool known = key == "udid" || key == "host_identifier" || key == "host_private_key" || key == "host_public_key"
                           || key == "advertised_identifier" || key == "peer_identifier" || key == "peer_public_key"
                           || key == "peer_alt_irk" || key == "remote_unlock_host_key";
        if (known && !seen_fields.insert(key).second) {
            err = MAA_IOS_TR("Duplicate pairing record field: ") + std::string(key);
            return std::nullopt;
        }
        auto set_hex = [&](Bytes& field, size_t want) -> bool {
            const std::optional<Bytes> bytes = unhex(value, err);
            if (!bytes) {
                return false;
            }
            if (bytes->size() != want) {
                err = MAA_IOS_TR("Incorrect field length: ") + std::string(key) + MAA_IOS_TR(" requires ") + std::to_string(want)
                      + MAA_IOS_TR(" bytes; got ") + std::to_string(bytes->size());
                return false;
            }
            field = *bytes;
            return true;
        };
        if (key == "udid") {
            record.udid = value;
        }
        else if (key == "host_identifier") {
            record.host_identifier = value;
        }
        else if (key == "host_private_key") {
            if (!set_hex(record.host_private_key, 32)) {
                return std::nullopt;
            }
            seen_private = true;
        }
        else if (key == "host_public_key") {
            if (!set_hex(record.host_public_key, 32)) {
                return std::nullopt;
            }
        }
        else if (key == "advertised_identifier") {
            record.advertised_identifier = value;
        }
        else if (key == "peer_identifier") {
            const std::optional<Bytes> identifier = unhex(value, err);
            if (!identifier) {
                return std::nullopt;
            }
            if (identifier->empty()) {
                err = MAA_IOS_TR("Pairing record peer_identifier must not be empty");
                return std::nullopt;
            }
            record.peer_identifier = *identifier;
        }
        else if (key == "peer_public_key") {
            if (!set_hex(record.peer_public_key, 32)) {
                return std::nullopt;
            }
        }
        else if (key == "peer_alt_irk") {
            if (!set_hex(record.peer_alt_irk, 16)) {
                return std::nullopt;
            }
        }
        else if (key == "remote_unlock_host_key") {
            record.remote_unlock_host_key = value;
        }
        // 忽略未知键，允许新增可选字段后的记录继续由旧版本解析。
    }
    if (!seen_private) {
        err = MAA_IOS_TR("Record missing host_private_key");
        return std::nullopt;
    }
    return record;
}

} // namespace maa::ios::wifi
