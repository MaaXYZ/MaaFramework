#include "InstanceStatus.h"

#include "Utils/Logger.h"

MAA_NS_BEGIN

const cv::Rect& InstanceStatus::get_pipeline_rec_box(const std::string& task) const
{
    auto it = pipeline_rec_box_map_.find(task);
    if (it == pipeline_rec_box_map_.end()) {
        static cv::Rect empty {};
        return empty;
    }
    return it->second;
}

void InstanceStatus::set_pipeline_rec_box(std::string task, cv::Rect rec)
{
    pipeline_rec_box_map_.insert_or_assign(std::move(task), std::move(rec));
}

void InstanceStatus::clear_pipeline_rec_box()
{
    LogInfo;

    pipeline_rec_box_map_.clear();
}

const json::value& InstanceStatus::get_pipeline_rec_detail(const std::string& task) const
{
    auto it = pipeline_rec_detail_map_.find(task);
    if (it == pipeline_rec_detail_map_.end()) {
        static json::value empty;
        return empty;
    }
    return it->second;
}

void InstanceStatus::set_pipeline_rec_detail(std::string task, json::value detail)
{
    pipeline_rec_detail_map_.insert_or_assign(std::move(task), std::move(detail));
}

void InstanceStatus::clear_pipeline_rec_detail()
{
    LogInfo;

    pipeline_rec_detail_map_.clear();
}

const json::value& InstanceStatus::get_pipeline_task_result(const std::string& task) const
{
    auto it = pipeline_task_result_map_.find(task);
    if (it == pipeline_task_result_map_.end()) {
        static json::value empty;
        return empty;
    }
    return it->second;
}

void InstanceStatus::set_pipeline_task_result(std::string task, json::value result)
{
    pipeline_task_result_map_.insert_or_assign(std::move(task), std::move(result));
}

void InstanceStatus::clear_pipeline_task_result()
{
    LogInfo;

    pipeline_task_result_map_.clear();
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
