#include "InstanceStatus.h"

#include "Utils/Logger.h"

MAA_NS_BEGIN

const cv::Rect& InstanceStatus::get_pipeline_rec_cache(const std::string& task) const
{
    auto it = pipeline_rec_cache_map_.find(task);
    if (it == pipeline_rec_cache_map_.end()) {
        static cv::Rect empty;
        return empty;
    }
    return it->second;
}

void InstanceStatus::set_pipeline_rec_cache(std::string task, cv::Rect rec)
{
    pipeline_rec_cache_map_.insert_or_assign(std::move(task), std::move(rec));
}

void InstanceStatus::clear_pipeline_rec_cache()
{
    LogInfo;

    pipeline_rec_cache_map_.clear();
}

uint64_t InstanceStatus::get_pipeline_run_times(const std::string& task) const
{
    auto it = pipeline_run_times_map_.find(task);
    if (it == pipeline_run_times_map_.end()) {
        return 0ULL;
    }
    return it->second;
}

void InstanceStatus::increase_pipeline_run_times(const std::string& task, int times)
{
    pipeline_run_times_map_[task] += times;
}

void InstanceStatus::clear_pipeline_run_times()
{
    LogInfo;

    pipeline_run_times_map_.clear();
}

MAA_NS_END
