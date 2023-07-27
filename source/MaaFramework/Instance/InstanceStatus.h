#pragma once

#include "Utils/NonCopyable.hpp"
#include "Conf/Conf.h"

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

    uint64_t get_pipeline_run_times(const std::string& task) const;
    void increase_pipeline_run_times(const std::string& task, int times = 1);
    void clear_pipeline_run_times();

private:
    std::map<std::string, cv::Rect> pipeline_rec_cache_map_;
    std::map<std::string, uint64_t> pipeline_run_times_map_;
};

MAA_NS_END
