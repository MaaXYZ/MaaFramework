#pragma once

#include "plist/Plist.h"
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace maa::ios::ddi
{
struct TssOptions
{
    std::chrono::milliseconds timeout { 60000 };
    std::function<bool()> cancelled;
};

std::optional<plist::Value> build_personalized_tss_request(
    const plist::Value& build_manifest,
    const plist::Value& identifiers,
    uint64_t ecid,
    const std::vector<uint8_t>& nonce,
    std::string& error);
std::optional<plist::Value> build_cryptex_tss_request(
    const plist::Value& build_identity,
    const plist::Value& chip_instance,
    const std::vector<uint8_t>& nonce,
    std::string& error);

// Tickets contain device-specific signing material. They remain in memory only.
std::optional<std::vector<uint8_t>> request_tss_ticket(const plist::Value& request, const TssOptions& options, std::string& error);

namespace detail
{
// Bounded response parser shared by the HTTPS transport and offline protocol tests.
std::optional<std::vector<uint8_t>> parse_tss_response(std::string_view response, bool cryptex, std::string& error);
}
} // namespace maa::ios::ddi
