#pragma once

#include <any>
#include <map>
#include <string>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "Utils/NoWarningCVMat.hpp"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

class InstanceStatus : public NonCopyable
{
public:
    std::any get_reco_hit(const std::string& name) const;
    void set_reco_hit(std::string task, std::any res);

    std::any get_reco_result(uint64_t uid) const;
    void set_reco_result(uint64_t uid, std::any res);
    void clear_reco();

    const json::value& get_task_result(const std::string& task) const;
    void set_task_result(std::string task, json::value result);
    void clear_task_result();

    uint64_t get_run_times(const std::string& task) const;
    void increase_run_times(const std::string& task, int times = 1);
    void clear_run_times();

public:
    void clear()
    {
        clear_reco();
        clear_task_result();
        clear_run_times();
    }

private:
    std::map<std::string, std::any> reco_hit_map_;
    std::map<uint64_t, std::any> reco_result_map_;

    std::map<std::string, json::value> task_result_map_;
    std::map<std::string, uint64_t> run_times_map_;
};

MAA_NS_END
