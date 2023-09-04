#pragma once

#include "Conf/Conf.h"
#include "Utils/NonCopyable.hpp"

#include <filesystem>
#include <map>

#include "Utils/NoWarningCVMat.hpp"

MAA_RES_NS_BEGIN

class TemplateResMgr : public NonCopyable
{
public:
    struct Paths
    {
        std::vector<std::filesystem::path> roots; // 类似环境变量，一级一级找，优先级高的在后面
        std::vector<std::string> filenames;       // 要找的，多个
    };

    bool lazy_load(const std::string& name, const std::filesystem::path& root,
                   const std::vector<std::string>& filenames);
    void clear();

public:
    const std::vector<cv::Mat>& get_template_images(const std::string& name) const;
    const std::vector<cv::Mat>& load_images(const std::string& name) const;
    cv::Mat load_single_image(const std::filesystem::path& path) const;

private:
    // for lazy load
    std::map<std::string, Paths> template_paths_;

    mutable std::map<std::string, std::vector<cv::Mat>> template_cache_;
    mutable std::map<std::filesystem::path, cv::Mat> template_bank_;
};

MAA_RES_NS_END
