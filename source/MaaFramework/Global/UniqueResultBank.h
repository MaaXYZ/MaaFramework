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
    std::any get_reco_result(uint64_t uid) const;
    void set_reco_result(uint64_t uid, std::any res);

    json::value get_task_result(uint64_t uid) const;
    void set_task_result(uint64_t uid, json::value result);

public:
    void clear();

private:
    std::map<uint64_t, std::any> reco_result_map_;
    std::map<uint64_t, json::value> task_result_map_;
};

MAA_NS_END
