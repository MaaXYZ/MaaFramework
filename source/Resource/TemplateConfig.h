#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"

#include <filesystem>
#include <map>

#include "Utils/NoWarningCVMat.h"

MAA_RES_NS_BEGIN

class TemplateConfig : public NonCopyable
{
public:
    bool lazy_load(const std::string& name, const std::filesystem::path& path);
    bool lazy_load(const std::map<std::string, std::filesystem::path>& path_map);
    void clear();

public:
    const cv::Mat& get_template_image(const std::string& name) const;

private:
    // for lazy load
    std::map<std::string, std::filesystem::path> template_paths_;
    mutable std::map<std::string, cv::Mat> template_cache_;
    mutable std::map<std::filesystem::path, cv::Mat> template_bank_;
};

MAA_RES_NS_END
