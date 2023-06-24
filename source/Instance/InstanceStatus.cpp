#include "InstanceStatus.h"

MAA_NS_BEGIN

const cv::Rect& InstanceStatus::get_pipeline_rec_cache(const std::string& pipeline) const
{
    auto it = pipeline_rec_cache_map_.find(pipeline);
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
    pipeline_rec_cache_map_.clear();
}

MAA_NS_END
