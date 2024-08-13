#pragma once

#include <any>
#include <map>
#include <optional>
#include <string>

#include "Utils/NoWarningCVMat.hpp"

MAA_NS_BEGIN

class RuntimeCache
{
public:
    cv::Rect get_pre_box(const std::string& task_name) const;
    void set_pre_box(std::string task_name, cv::Rect box);

    std::any get_reco_detail(int64_t uid) const;
    void add_reco_detail(int64_t uid, std::any detail);

    std::any get_node_detail(int64_t uid) const;
    void add_node_detail(int64_t uid, std::any detail);

    std::any get_task_detail(int64_t uid) const;
    void add_task_detail(int64_t uid, std::any detail);

    void clear();

private:
    std::map<std::string, cv::Rect> pre_boxes_;
    std::map<int64_t, std::any> reco_details_;
    std::map<int64_t, std::any> node_details_;
    std::map<int64_t, std::any> task_details_;
};

MAA_NS_END
