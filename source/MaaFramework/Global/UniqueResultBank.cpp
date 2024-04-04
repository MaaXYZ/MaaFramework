#include "UniqueResultBank.h"

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_NS_BEGIN

std::any UniqueResultBank::get_reco_detail(int64_t uid) const
{
    auto it = reco_detail_map_.find(uid);
    if (it == reco_detail_map_.end()) {
        return {};
    }
    return it->second;
}

void UniqueResultBank::add_reco_detail(int64_t uid, std::any detail)
{
    reco_detail_map_.insert_or_assign(uid, std::move(detail));
}

std::any UniqueResultBank::get_running_detail(int64_t uid) const
{
    auto it = task_detail_map_.find(uid);
    if (it == task_detail_map_.end()) {
        return {};
    }
    return it->second;
}

void UniqueResultBank::add_running_detail(int64_t uid, std::any detail)
{
    task_detail_map_.insert_or_assign(uid, std::move(detail));
}

void UniqueResultBank::clear()
{
    reco_detail_map_.clear();
    task_detail_map_.clear();
}

MAA_NS_END