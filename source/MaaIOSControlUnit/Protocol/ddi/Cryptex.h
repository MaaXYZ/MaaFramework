#pragma once
#include "ddi/Download.h"
#include "plist/Plist.h"
#include "remote/Device.h"
#include <array>
#include <chrono>
#include <functional>
#include <optional>

namespace maa::ios::ddi
{
struct CryptexInstallOptions
{
    std::chrono::seconds timeout { 300 };
    std::function<bool()> cancelled;
    std::function<void(const std::string&, uint64_t, uint64_t)> progress;
    bool replace_existing = false;
    std::function<std::optional<bool>(std::chrono::milliseconds, std::string&)> personalized_mounted;
    std::function<bool(std::chrono::milliseconds, std::string&)> remove_personalized;
};

struct CryptexInstallResult
{
    bool already_installed = false;
    std::string identifier;
    std::string version;
};

// Read-only cryptexd copy-installed. Does not sign, install, remove, mount,
// or change nonces. Empty result means the DDI identifier was not reported.
bool query_installed_cryptex(remote::Device&, std::optional<CryptexInstallResult>&, const CryptexInstallOptions&, std::string& error);

// Does not retry uncertain installs, or remove unrelated cryptexes. An explicit
// replacement is performed only after assets and an Apple ticket are ready.
std::optional<CryptexInstallResult>
    install_cryptex(remote::Device&, const DownloadResult&, const DownloadTarget&, const CryptexInstallOptions&, std::string& error);

namespace detail
{
struct CryptexMetadata
{
    plist::Value identity;
    xpc::Value properties;
    uint64_t nonce_domain = 0;
    std::string identifier, version;
};

std::optional<CryptexMetadata> parse_cryptex_metadata(const plist::Value& manifest, const plist::Value& info, std::string& error);
bool unwrap_cryptex_reply(const xpc::Value&, xpc::Value& argv, std::string& error);
std::optional<std::vector<uint8_t>> unwrap_cryptex_nonce(const std::vector<uint8_t>&, std::string& error);
xpc::Value cryptex_install_request(const CryptexMetadata&, const std::array<uint64_t, 5>& sizes);
} // namespace detail
} // namespace maa::ios::ddi
