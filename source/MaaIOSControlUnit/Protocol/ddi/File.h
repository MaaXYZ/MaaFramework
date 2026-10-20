#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace maa::ios::ddi::detail
{
std::optional<std::string> read_file(const std::filesystem::path& path, uint64_t limit, std::string& error);
} // namespace maa::ios::ddi::detail
