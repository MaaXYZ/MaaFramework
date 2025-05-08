#pragma once

#include <filesystem>
#include <map>

#include "Conf/Conf.h"
#include "Utils/NoWarningCVMat.hpp"
#include "Utils/NonCopyable.hpp"

MAA_RES_NS_BEGIN

class TemplateResMgr : public NonCopyable
{
public:
    using Image = cv::Mat;

    bool lazy_load(const std::filesystem::path& path);

    void clear();

public:
    std::shared_ptr<Image> image(const std::string& name);

private:
    std::shared_ptr<Image> load(const std::string& name);

    std::vector<std::filesystem::path> roots_;

    std::map<std::string, std::shared_ptr<Image>> images_;
};

MAA_RES_NS_END
