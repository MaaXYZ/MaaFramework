#include "InstanceStatus.h"

#include "Utils/Logger.h"

MAA_NS_BEGIN

const cv::Rect& InstanceStatus::get_rec_box(const std::string& task) const
{
    auto it = rec_box_map_.find(task);
    if (it == rec_box_map_.end()) {
        static cv::Rect empty {};
        return empty;
    }
    return it->second;
}

void InstanceStatus::set_rec_box(std::string task, cv::Rect rec)
{
    rec_box_map_.insert_or_assign(std::move(task), std::move(rec));
}

void InstanceStatus::clear_rec_box()
{
    LogDebug;

    rec_box_map_.clear();
}

const json::value& InstanceStatus::get_rec_detail(const std::string& task) const
{
    auto it = rec_detail_map_.find(task);
    if (it == rec_detail_map_.end()) {
        static json::value empty;
        return empty;
    }
    return it->second;
}

void InstanceStatus::set_rec_detail(std::string task, json::value detail)
{
    rec_detail_map_.insert_or_assign(std::move(task), std::move(detail));
}

void InstanceStatus::clear_rec_detail()
{
    LogDebug;

    rec_detail_map_.clear();
}

const json::value& InstanceStatus::get_task_result(const std::string& task) const
{
    auto it = task_result_map_.find(task);
    if (it == task_result_map_.end()) {
        static json::value empty;
        return empty;
    }
    return it->second;
}

void InstanceStatus::set_task_result(std::string task, json::value result)
{
    task_result_map_.insert_or_assign(std::move(task), std::move(result));
}

void InstanceStatus::clear_task_result()
{
    LogDebug;

    task_result_map_.clear();
}

uint64_t InstanceStatus::get_run_times(const std::string& task) const
{
    auto it = run_times_map_.find(task);
    if (it == run_times_map_.end()) {
        return 0ULL;
    }
    return it->second;
}

void InstanceStatus::increase_run_times(const std::string& task, int times)
{
    run_times_map_[task] += times;
}

void InstanceStatus::clear_run_times()
{
    LogDebug;

    run_times_map_.clear();
}

MAA_NS_END
