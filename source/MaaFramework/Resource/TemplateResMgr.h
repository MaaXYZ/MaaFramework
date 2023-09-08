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
    using Image = cv::Mat;

    bool lazy_load(const std::filesystem::path& path, bool is_base);

    void clear();

public:
    std::shared_ptr<Image> image(const std::string& name) const;

private:
    std::shared_ptr<Image> load(const std::string& name) const;

    std::vector<std::filesystem::path> roots_;

    mutable std::map<std::string, std::shared_ptr<Image>> images_;
};

MAA_RES_NS_END
