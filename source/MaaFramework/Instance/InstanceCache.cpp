#include "InstanceCache.h"

MAA_NS_BEGIN

cv::Rect InstanceCache::get_pre_task_box(const std::string& task_name) const
{
    auto it = pre_task_boxes_.find(task_name);
    if (it != pre_task_boxes_.end()) {
        return it->second;
    }
    return {};
}

void InstanceCache::set_pre_task_box(std::string task_name, cv::Rect box)
{
    pre_task_boxes_.insert_or_assign(std::move(task_name), std::move(box));
}

MAA_NS_END
