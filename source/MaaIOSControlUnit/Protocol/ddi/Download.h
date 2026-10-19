#pragma once

#include "ddi/Selection.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace maa::ios::ddi
{

struct DownloadOptions
{
    // Empty uses private temporary storage, removed with the last DownloadResult.
    // An explicit absolute root enables persistent validated caching.
    std::filesystem::path cache_root;
    std::chrono::seconds timeout { 300 };
    std::function<bool()> cancelled;
    // File name, bytes downloaded for this file, expected file size.
    std::function<void(const std::string&, uint64_t, uint64_t)> progress;
};

struct DownloadTarget
{
    std::string system_version = "27.0";
    std::string product_type;
    std::string hardware_model;
    std::optional<uint64_t> board_id;
    std::optional<uint64_t> chip_id;
};

struct DownloadResult
{
    std::filesystem::path directory;
    std::string build;
    std::string source_url;
    bool cache_hit = false;
    Kind kind = Kind::Cryptex;
    std::shared_ptr<void> temporary_storage = { };
};

// Selects the image category from the OS version. Personalized images validate
// any supplied model, board and chip against one manifest identity. Cryptex
// uses a generic identity; device support is checked during personalization and
// installation. Downloading alone does not establish MaaFramework feature support.
std::optional<DownloadResult> download_for_target(const DownloadOptions& options, const DownloadTarget& target, std::string& error);

// Downloads the pinned Cryptex DDI only. Does not connect to or modify a device.
// Returns nullopt and a diagnostic on failure, timeout or cancellation.
std::optional<DownloadResult> download_cryptex(const DownloadOptions& options, std::string& error);
// Validates every pinned file, its digest and the Cryptex manifest structure.
bool validate_cryptex_cache(const std::filesystem::path& directory, std::string& error);

} // namespace maa::ios::ddi
