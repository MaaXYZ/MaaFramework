#pragma once

#include <any>
#include <map>
#include <string>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "Utils/NoWarningCVMat.hpp"
#include "Utils/SingletonHolder.hpp"

MAA_NS_BEGIN

class UniqueResultBank : public SingletonHolder<UniqueResultBank>
{
    friend class SingletonHolder<UniqueResultBank>;

public:
    std::any get_reco_detail(int64_t uid) const;
    void add_reco_detail(int64_t uid, std::any detail);

    std::any get_running_detail(int64_t uid) const;
    void add_running_detail(int64_t uid, std::any detail);

public:
    void clear();

private:
    std::map<int64_t, std::any> reco_detail_map_;
    std::map<int64_t, std::any> task_detail_map_;
};

MAA_NS_END
