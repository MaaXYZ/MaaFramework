#include "plist/Plist.h"
#include "util/Base64.h"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <pugixml.hpp>
#include <sstream>
#include <stdexcept>

namespace maa::ios::plist
{
namespace
{
constexpr int kMaxDepth = 64;

bool whitespace(std::string_view text)
{
    return text.find_first_not_of(" \t\r\n") == std::string_view::npos;
}

// 容器只容许元素、格式空白及注释/处理指令。XML 语法由 pugixml 处理。
std::optional<std::vector<pugi::xml_node>> elements(pugi::xml_node node, std::string& error)
{
    std::vector<pugi::xml_node> result;
    for (auto child : node.children()) {
        if (child.type() == pugi::node_element) {
            result.push_back(child);
        }
        else if ((child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) && !whitespace(child.value())) {
            error = "Container contains nonempty text";
            return std::nullopt;
        }
    }
    return result;
}

std::optional<std::string> text(pugi::xml_node node, std::string& error)
{
    std::string result;
    for (auto child : node.children()) {
        if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) {
            result += child.value();
        }
        else if (child.type() == pugi::node_element) {
            error = "Scalar cannot contain nested elements";
            return std::nullopt;
        }
    }
    return result;
}

std::optional<Value> read_value(pugi::xml_node node, int depth, std::string& error)
{
    if (depth > kMaxDepth) {
        error = "Nesting too deep";
        return std::nullopt;
    }
    const std::string_view name = node.name();
    if (name == "dict" || name == "array") {
        const auto children = elements(node, error);
        if (!children) {
            return std::nullopt;
        }
        auto out = name == "dict" ? Value::Dict() : Value::Array();
        if (name == "array") {
            for (auto child : *children) {
                auto value = read_value(child, depth + 1, error);
                if (!value) {
                    return std::nullopt;
                }
                out.push(std::move(*value));
            }
            return out;
        }
        if (children->size() % 2 != 0) {
            error = "Dictionary key missing value";
            return std::nullopt;
        }
        for (size_t i = 0; i < children->size(); i += 2) {
            if (std::string_view((*children)[i].name()) != "key") {
                error = "Expected <key> in dictionary";
                return std::nullopt;
            }
            auto key = text((*children)[i], error);
            if (!key) {
                return std::nullopt;
            }
            auto value = read_value((*children)[i + 1], depth + 1, error);
            if (!value) {
                return std::nullopt;
            }
            out.set(std::move(*key), std::move(*value));
        }
        return out;
    }
    const auto content = text(node, error);
    if (!content) {
        return std::nullopt;
    }
    if (name == "true" || name == "false") {
        if (!whitespace(*content)) {
            error = "Boolean element must be empty";
            return std::nullopt;
        }
        return Value::Bool(name == "true");
    }
    if (name == "string") {
        return Value::Str(*content);
    }
    if (name == "data") {
        auto bytes = util::base64_decode(*content, error);
        if (!bytes) {
            error = "Invalid base64 in data: " + error;
            return std::nullopt;
        }
        return Value::OfData(std::move(*bytes));
    }
    if (name == "integer" || name == "real") {
        const auto first = content->find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            error = "Numeric value is empty";
            return std::nullopt;
        }
        auto number = std::string_view(*content).substr(first, content->find_last_not_of(" \t\r\n") - first + 1);
        if (name == "integer") {
            const bool negative = number.front() == '-';
            if (negative || number.front() == '+') {
                number.remove_prefix(1);
            }
            int base = 10;
            if (number.starts_with("0x") || number.starts_with("0X")) {
                base = 16;
                number.remove_prefix(2);
            }
            else if (number.starts_with('0')) {
                base = 8;
            }
            uint64_t value = 0;
            const auto [end, status] = std::from_chars(number.data(), number.data() + number.size(), value, base);
            if (status != std::errc() || end != number.data() + number.size() || (negative && value > (uint64_t { 1 } << 63))) {
                error = "Invalid or out-of-range integer";
                return std::nullopt;
            }
            if (negative) {
                return Value::Int(value == (uint64_t { 1 } << 63) ? INT64_MIN : -static_cast<int64_t>(value));
            }
            return value <= static_cast<uint64_t>(INT64_MAX) ? Value::Int(static_cast<int64_t>(value)) : Value::UInt(value);
        }
        char* end = nullptr;
        errno = 0;
        Value out;
        out.kind = Kind::Real;
        out.real = std::strtod(number.data(), &end);
        if (errno == ERANGE || end == number.data() || end != number.data() + number.size() || !std::isfinite(out.real)) {
            error = "Invalid or non-finite real";
            return std::nullopt;
        }
        return out;
    }
    error = "Unsupported element " + std::string(name);
    return std::nullopt;
}

const char* xml_text(const std::string& value)
{
    if (value.find('\0') != std::string::npos) {
        throw std::invalid_argument("XML text cannot contain NUL");
    }
    return value.c_str();
}

void write_value(pugi::xml_node parent, const Value& v, int depth)
{
    if (depth > kMaxDepth) {
        throw std::invalid_argument("plist nesting too deep");
    }
    switch (v.kind) {
    case Kind::Bool:
        parent.append_child(v.boolean ? "true" : "false");
        break;
    case Kind::Int:
        parent.append_child("integer").text().set(std::to_string(v.integer).c_str());
        break;
    case Kind::UInt:
        parent.append_child("integer").text().set(std::to_string(v.unsigned_integer).c_str());
        break;
    case Kind::Real:
        if (!std::isfinite(v.real)) {
            throw std::invalid_argument("plist real must be finite");
        }
        parent.append_child("real").text().set(v.real, 17);
        break;
    case Kind::String:
        parent.append_child("string").text().set(xml_text(v.string));
        break;
    case Kind::Data:
        parent.append_child("data").text().set(util::base64_encode(v.data).c_str());
        break;
    case Kind::Array: {
        auto node = parent.append_child("array");
        for (const auto& child : v.array) {
            write_value(node, child, depth + 1);
        }
        break;
    }
    case Kind::Dict: {
        if (v.keys.size() != v.values.size()) {
            throw std::invalid_argument("plist dictionary key/value count mismatch");
        }
        auto node = parent.append_child("dict");
        for (size_t i = 0; i < v.keys.size(); ++i) {
            node.append_child("key").text().set(xml_text(v.keys[i]));
            write_value(node, v.values[i], depth + 1);
        }
        break;
    }
    }
}
} // namespace

std::optional<Value> parse(std::string_view xml, std::string* err)
{
    std::string ignored;
    auto& error = err ? *err : ignored;
    error.clear();
    if (xml.size() > kMaxInput || xml.find('\0') != std::string_view::npos) {
        error = "XML input too large or contains NUL";
        return std::nullopt;
    }
    try {
        pugi::xml_document doc;
        // Preserve whitespace and root-level text so trailing garbage can be rejected.
        const auto result =
            doc.load_buffer(xml.data(), xml.size(), pugi::parse_full | pugi::parse_ws_pcdata | pugi::parse_fragment, pugi::encoding_utf8);
        if (!result) {
            error = result.description();
            return std::nullopt;
        }
        const auto roots = elements(doc, error);
        if (!roots) {
            return std::nullopt;
        }
        if (roots->size() != 1 || std::string_view((*roots)[0].name()) != "plist") {
            error = "Root must be a single <plist> element";
            return std::nullopt;
        }
        const auto values = elements((*roots)[0], error);
        if (!values) {
            return std::nullopt;
        }
        if (values->size() != 1) {
            error = "plist root must contain a single value";
            return std::nullopt;
        }
        return read_value((*values)[0], 0, error);
    }
    catch (const std::exception& e) {
        error = e.what();
        return std::nullopt;
    }
}

std::string write(const Value& v)
{
    pugi::xml_document doc;
    auto declaration = doc.append_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "UTF-8";
    doc.append_child(pugi::node_doctype)
        .set_value(
            "plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
            "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"");
    auto root = doc.append_child("plist");
    root.append_attribute("version") = "1.0";
    write_value(root, v, 0);
    std::ostringstream out;
    doc.save(out, "    ", pugi::format_default, pugi::encoding_utf8);
    return out.str();
}
} // namespace maa::ios::plist
