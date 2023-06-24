#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"

#include <map>
#include <string>

#include "Utils/NoWarningCVMat.h"

MAA_NS_BEGIN

class InstanceStatus : public NonCopyable
{
public:
    const cv::Rect& get_pipeline_rec_cache(const std::string& task) const;
    void set_pipeline_rec_cache(std::string task, cv::Rect rec);
    void clear_pipeline_rec_cache();

private:
    std::map<std::string, cv::Rect> pipeline_rec_cache_map_;
};

MAA_NS_END
