#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "NoWarningCV.hpp"

#include "File.hpp"
#include "Platform.h"

MAA_NS_BEGIN

inline cv::Mat imread(const std::filesystem::path& path, int flags = cv::IMREAD_COLOR)
{
    auto content = read_file<std::vector<uint8_t>>(path);
    return cv::imdecode(content, flags);
}

inline cv::Mat imread(const std::string& utf8_path, int flags = cv::IMREAD_COLOR)
{
    return imread(path(utf8_path), flags);
}

inline bool imwrite(const std::filesystem::path& path, cv::InputArray img,
                    const std::vector<int>& param = std::vector<int>())
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream of(path, std::ios::out | std::ios::binary);
    std::vector<uint8_t> encoded;
    auto ext = path_to_utf8_string(path.extension());
    if (cv::imencode(ext, img, encoded, param)) {
        of.write(reinterpret_cast<char*>(encoded.data()), encoded.size());
        return true;
    }
    return false;
}

inline bool imwrite(const std::string& utf8_path, cv::InputArray img,
                    const std::vector<int>& param = std::vector<int>())
{
    return imwrite(path(utf8_path), img, param);
}

MAA_NS_END
