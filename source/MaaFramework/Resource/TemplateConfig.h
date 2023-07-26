#pragma once

#include "Utils/NonCopyable.hpp"
#include "Conf/Conf.h"

#include <filesystem>
#include <map>

#include "Utils/NoWarningCVMat.h"

MAA_RES_NS_BEGIN

class TemplateConfig : public NonCopyable
{
public:
    bool lazy_load(const std::string& name, const std::vector<std::filesystem::path>& paths);
    void clear();

public:
    const std::vector<cv::Mat>& get_template_images(const std::string& name) const;

private:
    // for lazy load
    using Paths = std::map<std::string, std::vector<std::filesystem::path>>;
    Paths template_paths_;

    mutable std::map<std::string, std::vector<cv::Mat>> template_cache_;
    mutable std::map<std::filesystem::path, cv::Mat> template_bank_;
};

MAA_RES_NS_END
