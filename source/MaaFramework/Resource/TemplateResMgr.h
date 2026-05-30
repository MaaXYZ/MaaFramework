#pragma once

#include <filesystem>
#include <mutex>
#include <unordered_map>

#include "Common/Conf.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "MaaUtils/NonCopyable.hpp"

MAA_RES_NS_BEGIN

class TemplateResMgr : public NonCopyable
{
public:
    bool lazy_load(const std::filesystem::path& path);
    bool load_file(const std::filesystem::path& path);

    void clear();

public:
    std::vector<cv::Mat> get_image(const std::string& name);
    void set_image(const std::string& name, const cv::Mat& image);

private:
    std::vector<cv::Mat> load(const std::string& name);

    std::vector<std::filesystem::path> roots_ = { "" }; // for filepath without prefix

    // 保护 image_cache_ 的懒加载写入，使其可被并行识别安全访问。
    std::mutex image_cache_mutex_;
    std::unordered_map<std::string, std::vector<cv::Mat>> image_cache_;
};

MAA_RES_NS_END
