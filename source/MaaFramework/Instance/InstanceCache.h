#pragma once

#include <map>
#include <string>

#include "Utils/NoWarningCVMat.hpp"

MAA_NS_BEGIN

class InstanceCache
{
public:
    cv::Rect get_pre_task_box(const std::string& task_name) const;
    void set_pre_task_box(std::string task_name, cv::Rect box);

private:
    std::map<std::string, cv::Rect> pre_task_boxes_;
};

MAA_NS_END
