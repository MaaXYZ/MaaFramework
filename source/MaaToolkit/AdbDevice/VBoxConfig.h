#pragma once

#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <boost/asio/ip/address.hpp>

#include "rapidxml/rapidxml.hpp"

namespace MaaNS::ToolkitNS
{
namespace vbox_config_detail
{
using XmlNode = boost::property_tree::detail::rapidxml::xml_node<char>;

inline std::string_view local_name(std::string_view name)
{
    const auto colon = name.find(':');
    return colon == std::string_view::npos ? name : name.substr(colon + 1);
}

inline const XmlNode* child(const XmlNode& node, std::string_view name)
{
    for (const auto* item = node.first_node(); item; item = item->next_sibling()) {
        if (local_name(std::string_view(item->name(), item->name_size())) == name) {
            return item;
        }
    }
    return nullptr;
}

inline std::string attribute(const XmlNode& node, std::string_view name)
{
    for (const auto* item = node.first_attribute(); item; item = item->next_attribute()) {
        if (local_name(std::string_view(item->name(), item->name_size())) == name) {
            return std::string(item->value(), item->value_size());
        }
    }
    return { };
}

inline std::optional<unsigned int> port_number(std::string_view value)
{
    unsigned int port = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), port);
    if (error != std::errc { } || end != value.data() + value.size() || port == 0 || port > 65535) {
        return std::nullopt;
    }
    return port;
}

inline std::optional<std::string> forwarding_serial(const XmlNode& forwarding)
{
    if (attribute(forwarding, "proto") != "1" || port_number(attribute(forwarding, "guestport")) != 5555) {
        return std::nullopt;
    }
    const auto port = port_number(attribute(forwarding, "hostport"));
    if (!port) {
        return std::nullopt;
    }

    std::string host = attribute(forwarding, "hostip");
    if (host.empty()) {
        host = "127.0.0.1";
    }
    boost::system::error_code error;
    auto address = boost::asio::ip::make_address(host, error);
    if (error) {
        return std::nullopt;
    }
    if (address.is_unspecified()) {
        address = address.is_v4() ? boost::asio::ip::address(boost::asio::ip::address_v4::loopback())
                                  : boost::asio::ip::address(boost::asio::ip::address_v6::loopback());
    }
    host = address.to_string();
    if (address.is_v6()) {
        host = '[' + host + ']';
    }
    return host + ':' + std::to_string(*port);
}

inline std::vector<std::string> serials(const XmlNode& document)
{
    const auto* root = document.first_node();
    if (!root || local_name(std::string_view(root->name(), root->name_size())) != "VirtualBox" || root->next_sibling()) {
        return { };
    }
    const auto* machine = child(*root, "Machine");
    const auto* hardware = machine ? child(*machine, "Hardware") : nullptr;
    const auto* network = hardware ? child(*hardware, "Network") : nullptr;
    if (!network) {
        return { };
    }

    std::vector<std::string> result;
    std::unordered_set<std::string> seen;
    for (const auto* adapter = network->first_node(); adapter; adapter = adapter->next_sibling()) {
        const auto enabled = attribute(*adapter, "enabled");
        if (local_name(std::string_view(adapter->name(), adapter->name_size())) != "Adapter" || (enabled != "true" && enabled != "1")) {
            continue;
        }
        const auto* nat = child(*adapter, "NAT");
        if (!nat) {
            continue;
        }
        for (const auto* rule = nat->first_node(); rule; rule = rule->next_sibling()) {
            if (local_name(std::string_view(rule->name(), rule->name_size())) != "Forwarding") {
                continue;
            }
            auto serial = forwarding_serial(*rule);
            if (serial && seen.emplace(*serial).second) {
                result.emplace_back(std::move(*serial));
            }
        }
    }
    return result;
}
} // namespace vbox_config_detail

inline std::vector<std::string> read_vbox_adb_serials(const std::filesystem::path& path)
{
    // VM settings may be incomplete during an emulator's atomic-save sequence.
    // Limit input size before handing it to the XML parser and treat failed reads as a miss.
    constexpr std::size_t kMaxConfigSize = 4 * 1024 * 1024;
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return { };
    }
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    if (size < 0 || size > static_cast<std::streamoff>(kMaxConfigSize)) {
        return { };
    }
    input.seekg(0, std::ios::beg);
    std::string content(static_cast<std::size_t>(size) + 1, '\0');
    input.read(content.data(), static_cast<std::streamsize>(content.size()));
    if (input.bad() || input.gcount() != static_cast<std::streamsize>(size)) {
        return { };
    }
    content.resize(static_cast<std::size_t>(size));
    if (content.find('\0') != std::string::npos) {
        return { };
    }

    namespace xml = boost::property_tree::detail::rapidxml;
    xml::xml_document<char> document;
    try {
        document.parse<xml::parse_validate_closing_tags>(content.data());
    }
    catch (const xml::parse_error&) {
        return { };
    }
    return vbox_config_detail::serials(document);
}
} // namespace MaaNS::ToolkitNS
