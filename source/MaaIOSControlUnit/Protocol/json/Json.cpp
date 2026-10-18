#include "json/Json.h"

#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace maa::ios::json
{
namespace
{
size_t utf8_width(std::string_view text, size_t offset)
{
    const auto first = static_cast<unsigned char>(text[offset]);
    if (first < 0x80) {
        return 1;
    }
    const size_t width = first >= 0xc2 && first <= 0xdf ? 2 : first >= 0xe0 && first <= 0xef ? 3 : first >= 0xf0 && first <= 0xf4 ? 4 : 0;
    if (!width || width > text.size() - offset) {
        return 0;
    }
    uint32_t codepoint = first & (0x7f >> width);
    for (size_t i = 1; i < width; ++i) {
        const auto next = static_cast<unsigned char>(text[offset + i]);
        if ((next & 0xc0) != 0x80) {
            return 0;
        }
        codepoint = (codepoint << 6) | (next & 0x3f);
    }
    const uint32_t minimum = width == 2 ? 0x80 : width == 3 ? 0x800 : 0x10000;
    return codepoint >= minimum && codepoint <= 0x10ffff && (codepoint < 0xd800 || codepoint > 0xdfff) ? width : 0;
}

bool bounded_input(std::string_view text, std::string& error)
{
    size_t depth = 0;
    bool quoted = false;
    bool escaped = false;
    for (size_t i = 0; i < text.size();) {
        const auto width = utf8_width(text, i);
        if (!width) {
            error = "Invalid UTF-8 in JSON";
            return false;
        }
        const auto ch = static_cast<unsigned char>(text[i]);
        i += width;
        if (quoted) {
            if (ch < 0x20) {
                error = "Unescaped control character in JSON string";
                return false;
            }
            if (escaped) {
                escaped = false;
            }
            else if (ch == '\\') {
                escaped = true;
            }
            else if (ch == '"') {
                quoted = false;
            }
            continue;
        }
        if (ch == '"') {
            quoted = true;
        }
        else if (ch == '[' || ch == '{') {
            if (++depth > 64) {
                error = "Nesting too deep";
                return false;
            }
        }
        else if (ch == ']' || ch == '}') {
            if (!depth) {
                error = "Unmatched JSON container";
                return false;
            }
            --depth;
        }
        else if (ch < 0x20 && ch != '\t' && ch != '\r' && ch != '\n') {
            error = "Invalid JSON whitespace";
            return false;
        }
    }
    return true;
}

bool finite_number(std::string_view text, double& number)
{
    std::istringstream stream { std::string(text) };
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> number;
    return stream && stream.eof() && std::isfinite(number);
}

bool finite_numbers(const Value& value)
{
    if (value.is_number() && !value.is<int64_t>() && !value.is<uint64_t>()) {
        return as_double(value).has_value();
    }
    if (value.is_array()) {
        for (const auto& child : value.as_array()) {
            if (!finite_numbers(child)) {
                return false;
            }
        }
    }
    if (value.is_object()) {
        for (const auto& [key, child] : value.as_object()) {
            if (!finite_numbers(child)) {
                return false;
            }
        }
    }
    return true;
}
}

std::optional<Value> parse(std::string_view text, std::string* err)
{
    if (err) {
        err->clear();
    }
    const auto content = text.starts_with("\xef\xbb\xbf") ? text.substr(3) : text;
    std::string error;
    if (text.size() > (4u << 20)) {
        error = "Input too large";
    }
    else if (bounded_input(content, error)) {
        // meojson parses containers at the root. Wrapping also permits scalar protocol values.
        std::string wrapped;
        wrapped.reserve(content.size() + 2);
        wrapped += '[';
        wrapped += content;
        wrapped += ']';
        auto parsed = ::json::parse(wrapped);
        if (parsed && parsed->as_array().size() == 1 && finite_numbers(parsed->at(0))) {
            return std::move(parsed->as_array()[0]);
        }
        error = "Invalid JSON";
    }
    if (err) {
        *err = std::move(error);
    }
    return std::nullopt;
}

std::string write(const Value& value, bool replace_invalid_utf8)
{
    const auto text = value.to_string();
    std::string output;
    output.reserve(text.size());
    for (size_t i = 0; i < text.size();) {
        const auto width = utf8_width(text, i);
        if (width) {
            output.append(text, i, width);
            i += width;
            continue;
        }
        if (!replace_invalid_utf8) {
            throw std::runtime_error("Invalid UTF-8 in JSON output");
        }
        output += "\xef\xbf\xbd";
        ++i;
    }
    return output;
}

const Value* find(const Value& value, std::string_view key)
{
    return value.find_value(std::string(key));
}

std::string as_string_or(const Value& value, std::string_view fallback)
{
    return value.is_string() ? value.as_string() : std::string(fallback);
}

std::optional<double> as_double(const Value& value)
{
    if (!value.is_number()) {
        return std::nullopt;
    }
    double number = 0;
    if (!finite_number(value.to_string(), number)) {
        return std::nullopt;
    }
    return number;
}

int64_t as_int_or(const Value& value, int64_t fallback)
{
    if (value.is<int64_t>()) {
        return value.as<int64_t>();
    }
    if (!value.is_number() || value.is<uint64_t>()) {
        return fallback;
    }
    const auto number = as_double(value);
    if (number && *number >= -0x1p63 && *number < 0x1p63) {
        return static_cast<int64_t>(*number);
    }
    return fallback;
}
} // namespace maa::ios::json
