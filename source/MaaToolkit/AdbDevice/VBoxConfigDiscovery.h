#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

#include "VBoxConfig.h"

namespace MaaNS::ToolkitNS
{
namespace vbox_detail
{
inline void collect_serials(
    const std::filesystem::path& directory,
    const std::filesystem::path& extension,
    int depth,
    size_t& remaining_entries,
    std::unordered_set<std::string>& seen,
    std::vector<std::string>& result)
{
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(directory, ec);
    if (ec || !std::filesystem::is_directory(status)) {
        return;
    }

    std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied, ec);
    const std::filesystem::directory_iterator end;
    while (!ec && it != end && remaining_entries > 0) {
        --remaining_entries;
        const auto entry_status = it->symlink_status(ec);
        if (!ec && std::filesystem::is_directory(entry_status) && depth > 0) {
            collect_serials(it->path(), extension, depth - 1, remaining_entries, seen, result);
        }
        else if (!ec && std::filesystem::is_regular_file(entry_status)) {
            auto suffix = it->path().extension().native();
            for (auto& ch : suffix) {
                if (ch >= 'A' && ch <= 'Z') {
                    ch = static_cast<std::filesystem::path::value_type>(ch + ('a' - 'A'));
                }
            }
            if (suffix == extension.native()) {
                for (auto& serial : read_vbox_adb_serials(it->path())) {
                    if (seen.emplace(serial).second) {
                        result.emplace_back(std::move(serial));
                    }
                }
            }
        }
        // An unreadable entry must not prevent discovery of the remaining instances.
        ec.clear();
        it.increment(ec);
    }
}
} // namespace vbox_detail

inline std::vector<std::string> find_vbox_adb_serials(const std::string& emulator_name, const std::filesystem::path& process_path)
{
    if (process_path.empty()) {
        return { };
    }

    const auto dir = process_path.parent_path();
    std::vector<std::filesystem::path> roots;
    std::filesystem::path extension;
    if (emulator_name == "Nox") {
        roots = { dir / "BignoxVMS" };
        extension = ".vbox";
    }
    else if (emulator_name == "MEmuPlayer") {
        roots = { dir / "MemuHyperv VMs" };
        extension = ".memu";
    }
    else if (emulator_name == "MuMuPlayer v3") {
        roots = { dir / "vms", dir.parent_path() / "vms" };
        extension = ".nemu";
    }
    else {
        return { };
    }

    std::vector<std::string> result;
    std::unordered_set<std::string> seen;
    // Only inspect known config roots and their immediate instance directories.
    // A shared budget also bounds work for installations with many stale instances.
    size_t remaining_entries = 4096;
    for (const auto& root : roots) {
        vbox_detail::collect_serials(root, extension, 1, remaining_entries, seen, result);
    }
    return result;
}
} // namespace MaaNS::ToolkitNS
