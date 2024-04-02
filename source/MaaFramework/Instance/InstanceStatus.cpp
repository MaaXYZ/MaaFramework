#include "InstanceStatus.h"

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_NS_BEGIN

std::any InstanceStatus::get_reco_hit(const std::string& name) const
{
    auto it = reco_hit_map_.find(name);
    if (it == reco_hit_map_.end()) {
        return {};
    }
    return it->second;
}

void InstanceStatus::set_reco_hit(std::string task, std::any res)
{
    reco_hit_map_.insert_or_assign(std::move(task), std::move(res));
}

std::any InstanceStatus::get_reco_result(uint64_t uid) const
{
    auto it = reco_result_map_.find(uid);
    if (it == reco_result_map_.end()) {
        return {};
    }
    return it->second;
}

void InstanceStatus::set_reco_result(uint64_t uid, std::any res)
{
    reco_result_map_.insert_or_assign(uid, std::move(res));
}

void InstanceStatus::clear_reco()
{
    reco_result_map_.clear();
    reco_hit_map_.clear();
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
    LogTrace;

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
    LogTrace;

    run_times_map_.clear();
}

MAA_NS_END
