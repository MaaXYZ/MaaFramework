#include "ddi/File.h"

#include <fstream>

namespace maa::ios::ddi::detail
{
std::optional<std::string> read_file(const std::filesystem::path& path, uint64_t limit, std::string& error)
{
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(path)) || !std::filesystem::is_regular_file(path)) {
        error = "DDI cache contains a nonregular file";
        return std::nullopt;
    }
    const auto size = std::filesystem::file_size(path);
    if (size > limit) {
        error = "DDI file exceeds its size limit";
        return std::nullopt;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "Cannot open DDI file";
        return std::nullopt;
    }
    std::string bytes(static_cast<std::size_t>(size), '\0');
    file.read(bytes.data(), static_cast<std::streamsize>(size));
    if (!file || file.gcount() != static_cast<std::streamsize>(size)) {
        error = "Cannot read complete DDI file";
        return std::nullopt;
    }
    return bytes;
}
} // namespace maa::ios::ddi::detail
