#include "TaskCache.h"

MAA_NS_BEGIN

std::optional<cv::Rect> TaskCache::get_pre_box(const std::string& task_name) const
{
    auto it = pre_boxes_.find(task_name);
    if (it == pre_boxes_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void TaskCache::set_pre_box(std::string task_name, cv::Rect box)
{
    pre_boxes_.insert_or_assign(std::move(task_name), std::move(box));
}

MAA_NS_END
