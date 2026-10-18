#include "plist/Plist.h"
#include "util/Base64.h"

#include <cmath>
#include <pugixml.hpp>
#include <sstream>
#include <stdexcept>

namespace maa::ios::plist
{
namespace
{
constexpr int kMaxDepth = 64;
constexpr size_t kMaxInput = 8u << 20;

bool whitespace(std::string_view text)
{
    return text.find_first_not_of(" \t\r\n") == std::string_view::npos;
}

// 容器只容许元素、格式空白及注释/处理指令。XML 语法由 pugixml 处理。
std::vector<pugi::xml_node> elements(pugi::xml_node node)
{
    std::vector<pugi::xml_node> result;
    for (auto child : node.children()) {
        if (child.type() == pugi::node_element) {
            result.push_back(child);
        }
        else if ((child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) && !whitespace(child.value())) {
            throw std::runtime_error("Container contains nonempty text");
        }
    }
    return result;
}

std::string text(pugi::xml_node node)
{
    std::string result;
    for (auto child : node.children()) {
        if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) {
            result += child.value();
        }
        else if (child.type() == pugi::node_element) {
            throw std::runtime_error("Scalar cannot contain nested elements");
        }
    }
    return result;
}

Value read_value(pugi::xml_node node, int depth)
{
    if (depth > kMaxDepth) {
        throw std::runtime_error("Nesting too deep");
    }
    const std::string_view name = node.name();
    if (name == "dict" || name == "array") {
        const auto children = elements(node);
        auto out = name == "dict" ? Value::Dict() : Value::Array();
        if (name == "array") {
            for (auto child : children) {
                out.push(read_value(child, depth + 1));
            }
        }
        else {
            if (children.size() % 2 != 0) {
                throw std::runtime_error("Dictionary key missing value");
            }
            for (size_t i = 0; i < children.size(); i += 2) {
                if (std::string_view(children[i].name()) != "key") {
                    throw std::runtime_error("Expected <key> in dictionary");
                }
                out.set(text(children[i]), read_value(children[i + 1], depth + 1));
            }
        }
        return out;
    }
    const auto content = text(node);
    if (name == "true" || name == "false") {
        if (!whitespace(content)) {
            throw std::runtime_error("Boolean element must be empty");
        }
        return Value::Bool(name == "true");
    }
    if (name == "string") {
        return Value::Str(content);
    }
    if (name == "data") {
        std::string err;
        auto bytes = util::base64_decode(content, err);
        if (!bytes) {
            throw std::runtime_error("Invalid base64 in data: " + err);
        }
        return Value::OfData(std::move(*bytes));
    }
    if (name == "integer" || name == "real") {
        const auto first = content.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            throw std::runtime_error("Numeric value is empty");
        }
        const auto number = content.substr(first, content.find_last_not_of(" \t\r\n") - first + 1);
        size_t end = 0;
        Value out;
        if (name == "integer") {
            if (number.front() == '-') {
                out = Value::Int(std::stoll(number, &end, 0));
            }
            else {
                const auto value = std::stoull(number, &end, 0);
                out = value <= static_cast<uint64_t>(INT64_MAX) ? Value::Int(static_cast<int64_t>(value)) : Value::UInt(value);
            }
        }
        else {
            out.kind = Kind::Real;
            out.real = std::stod(number, &end);
            if (!std::isfinite(out.real)) {
                throw std::runtime_error("Real must be finite");
            }
        }
        if (end != number.size()) {
            throw std::runtime_error("Trailing content in numeric value");
        }
        return out;
    }
    throw std::runtime_error("Unsupported element " + std::string(name));
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
    if (err) {
        err->clear();
    }
    if (xml.size() > kMaxInput || xml.find('\0') != std::string_view::npos) {
        if (err) {
            *err = "XML input too large or contains NUL";
        }
        return std::nullopt;
    }
    try {
        pugi::xml_document doc;
        // 保留文本空白；fragment 让根外文本也进入 DOM，以便拒绝尾部垃圾。
        const auto result =
            doc.load_buffer(xml.data(), xml.size(), pugi::parse_full | pugi::parse_ws_pcdata | pugi::parse_fragment, pugi::encoding_utf8);
        if (!result) {
            throw std::runtime_error(result.description());
        }
        const auto roots = elements(doc);
        if (roots.size() != 1 || std::string_view(roots[0].name()) != "plist") {
            throw std::runtime_error("Root must be a single <plist> element");
        }
        const auto values = elements(roots[0]);
        if (values.size() != 1) {
            throw std::runtime_error("plist root must contain a single value");
        }
        return read_value(values[0], 0);
    }
    catch (const std::exception& e) {
        if (err) {
            *err = e.what();
        }
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
