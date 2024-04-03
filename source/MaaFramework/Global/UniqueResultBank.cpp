#include "UniqueResultBank.h"

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_NS_BEGIN

std::any UniqueResultBank::get_reco_result(uint64_t uid) const
{
    auto it = reco_result_map_.find(uid);
    if (it == reco_result_map_.end()) {
        return {};
    }
    return it->second;
}

void UniqueResultBank::set_reco_result(uint64_t uid, std::any res)
{
    reco_result_map_.insert_or_assign(uid, std::move(res));
}

json::value UniqueResultBank::get_task_result(uint64_t uid) const
{
    auto it = task_result_map_.find(uid);
    if (it == task_result_map_.end()) {
        return {};
    }
    return it->second;
}

void UniqueResultBank::set_task_result(uint64_t uid, json::value result)
{
    task_result_map_.insert_or_assign(uid, std::move(result));
}

void UniqueResultBank::clear()
{
    reco_result_map_.clear();
    task_result_map_.clear();
}

MAA_NS_END
