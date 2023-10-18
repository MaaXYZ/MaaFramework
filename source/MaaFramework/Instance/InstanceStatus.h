#pragma once

#include "Conf/Conf.h"
#include "Utils/NonCopyable.hpp"

#include <map>
#include <string>

#include "Utils/NoWarningCVMat.hpp"

#include <meojson/json.hpp>

MAA_NS_BEGIN

class InstanceStatus : public NonCopyable
{
public:
    const cv::Rect& get_rec_box(const std::string& task) const;
    void set_rec_box(std::string task, cv::Rect rec);
    void clear_rec_box();

    const json::value& get_rec_detail(const std::string& task) const;
    void set_rec_detail(std::string task, json::value detail);
    void clear_rec_detail();

    const json::value& get_task_result(const std::string& task) const;
    void set_task_result(std::string task, json::value result);
    void clear_task_result();

    uint64_t get_run_times(const std::string& task) const;
    void increase_run_times(const std::string& task, int times = 1);
    void clear_run_times();

    void clear()
    {
        clear_rec_box();
        clear_rec_detail();
        clear_task_result();
        clear_run_times();
    }

private:
    std::map<std::string, cv::Rect> rec_box_map_;
    std::map<std::string, json::value> rec_detail_map_;
    std::map<std::string, json::value> task_result_map_;
    std::map<std::string, uint64_t> run_times_map_;
};

MAA_NS_END
