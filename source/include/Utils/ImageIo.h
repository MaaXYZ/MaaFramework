#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Utils/NoWarningCV.hpp"

#include "Utils/File.hpp"
#include "Utils/Platform.h"

MAA_NS_BEGIN

inline cv::Mat imread(const std::filesystem::path& path, int flags = cv::IMREAD_COLOR)
{
    auto content = read_file<std::vector<uint8_t>>(path);
    if (content.empty()) {
        return {};
    }
    return cv::imdecode(content, flags);
}

inline cv::Mat imread(const std::string& utf8_path, int flags = cv::IMREAD_COLOR)
{
    return imread(path(utf8_path), flags);
}

inline bool imwrite(const std::filesystem::path& path, cv::InputArray img)
{
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    auto ext = path_to_utf8_string(path.extension());
    std::vector<uint8_t> encoded;
    if (!cv::imencode(ext, img, encoded)) {
        return false;
    }

    std::ofstream of(path, std::ios::out | std::ios::binary);
    of.write(reinterpret_cast<char*>(encoded.data()), encoded.size());
    return true;
}

inline bool imwrite(const std::string& utf8_path, cv::InputArray img)
{
    return imwrite(path(utf8_path), img);
}

inline bool imwrite(const char* utf8_path, cv::InputArray img)
{
    return imwrite(path(utf8_path), img);
}

MAA_NS_END
