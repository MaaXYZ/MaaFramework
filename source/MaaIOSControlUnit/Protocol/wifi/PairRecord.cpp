#include "wifi/PairRecord.h"
#include "util/Hex.h"

#include <set>

namespace maa::ios::wifi
{
namespace
{

constexpr std::string_view kHeader = "MaaIOS1";

std::string encode_text(std::string_view value)
{
    std::string result;
    for (const char ch : value) {
        if (ch == '%') {
            result += "%25";
        }
        else if (ch == '|') {
            result += "%7C";
        }
        else {
            result += ch;
        }
    }
    return result;
}

std::optional<std::string> decode_text(std::string_view value, std::string& error)
{
    std::string result;
    for (size_t i = 0; i < value.size(); ++i) {
        char ch = value[i];
        if (ch == '%') {
            if (i + 2 >= value.size()) {
                error = "Invalid pairing record text escape";
                return std::nullopt;
            }
            auto byte = util::hex_decode(value.substr(i + 1, 2), error);
            if (!byte) {
                return std::nullopt;
            }
            ch = static_cast<char>((*byte)[0]);
            i += 2;
        }
        if (ch == 0 || ch == '\r' || ch == '\n') {
            error = "Pairing record identity must not contain NUL or line breaks";
            return std::nullopt;
        }
        result += ch;
    }
    return result;
}

} // namespace

std::string format_record(const PairRecord& record)
{
    std::string out(kHeader);
    const auto append_text = [&](std::string_view key, std::string_view value) {
        out += '|';
        out += key;
        out += '=';
        out += encode_text(value);
    };
    append_text("udid", record.udid);
    append_text("hid", record.host_identifier);
    if (!record.peer_identifier.empty()) {
        append_text("pid", record.peer_identifier);
    }
    append_text("hsk", util::hex_encode(record.host_private_key));
    append_text("hpk", util::hex_encode(record.host_public_key));
    if (!record.peer_public_key.empty()) {
        append_text("ppk", util::hex_encode(record.peer_public_key));
    }
    if (!record.peer_alt_irk.empty()) {
        append_text("irk", util::hex_encode(record.peer_alt_irk));
    }
    return out;
}

std::optional<PairRecord> parse_record(std::string_view text, std::string& err)
{
    err.clear();
    if (text.size() <= kHeader.size() || text.substr(0, kHeader.size()) != kHeader || text[kHeader.size()] != '|') {
        err = "Invalid MaaIOS pairing record header";
        return std::nullopt;
    }
    if (text.find_first_of("\r\n") != std::string_view::npos || text.find('\0') != std::string_view::npos || text.back() == '|') {
        err = "Pairing record must be a single line of nonempty fields";
        return std::nullopt;
    }
    PairRecord record;
    bool seen_private = false;
    std::set<std::string_view> seen_fields;
    size_t pos = kHeader.size() + 1;
    while (pos < text.size()) {
        size_t end = text.find('|', pos);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        const std::string_view field = text.substr(pos, end - pos);
        pos = end + 1;
        const size_t eq = field.find('=');
        if (eq == std::string_view::npos || eq == 0) {
            err = "Pairing record field must be key=value";
            return std::nullopt;
        }
        const auto key = field.substr(0, eq);
        const auto value = field.substr(eq + 1);
        if (!seen_fields.insert(key).second) {
            err = "Duplicate pairing record field: " + std::string(key);
            return std::nullopt;
        }
        const auto set_text = [&](std::string& target) {
            auto decoded = decode_text(value, err);
            if (!decoded) {
                return false;
            }
            target = std::move(*decoded);
            return true;
        };
        auto set_hex = [&](Bytes& field, size_t want) -> bool {
            const std::optional<Bytes> bytes = util::hex_decode(value, err);
            if (!bytes) {
                return false;
            }
            if (bytes->size() != want) {
                err = "Incorrect field length: " + std::string(key) + " requires " + std::to_string(want) + " bytes; got "
                      + std::to_string(bytes->size());
                return false;
            }
            field = *bytes;
            return true;
        };
        if (key == "udid") {
            if (!set_text(record.udid)) {
                return std::nullopt;
            }
        }
        else if (key == "hid") {
            if (!set_text(record.host_identifier)) {
                return std::nullopt;
            }
        }
        else if (key == "pid") {
            if (!set_text(record.peer_identifier)) {
                return std::nullopt;
            }
            if (record.peer_identifier.empty() || record.peer_identifier.size() > kMaxPeerIdentifier) {
                err = "Pairing record pid must be nonempty text";
                return std::nullopt;
            }
        }
        else if (key == "hsk") {
            if (!set_hex(record.host_private_key, 32)) {
                return std::nullopt;
            }
            seen_private = true;
        }
        else if (key == "hpk") {
            if (!set_hex(record.host_public_key, 32)) {
                return std::nullopt;
            }
        }
        else if (key == "ppk") {
            if (!set_hex(record.peer_public_key, 32)) {
                return std::nullopt;
            }
        }
        else if (key == "irk") {
            if (!set_hex(record.peer_alt_irk, 16)) {
                return std::nullopt;
            }
        }
        else {
            err = "Unknown pairing record field: " + std::string(key);
            return std::nullopt;
        }
    }
    if (!seen_private) {
        err = "Record missing hsk";
        return std::nullopt;
    }
    return record;
}

} // namespace maa::ios::wifi
