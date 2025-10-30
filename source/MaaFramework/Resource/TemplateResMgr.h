#pragma once

#include <filesystem>
#include <map>

#include "Common/Conf.h"
#include "Utils/NoWarningCVMat.hpp"
#include "Utils/NonCopyable.hpp"

MAA_RES_NS_BEGIN

class TemplateResMgr : public NonCopyable
{
public:
    bool lazy_load(const std::filesystem::path& path);

    void clear();

public:
    std::vector<cv::Mat> images(const std::vector<std::string>& names);

private:
    std::vector<cv::Mat> load(const std::string& name);

    std::vector<std::filesystem::path> roots_ = { "" }; // for filepath without prefix

    std::map<std::string, std::vector<cv::Mat>> image_cahce_;
};

MAA_RES_NS_END
