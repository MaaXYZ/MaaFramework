#pragma once

#include <map>
#include <optional>
#include <string>

#include "Utils/NoWarningCVMat.hpp"

MAA_NS_BEGIN

class TaskCache
{
public:
    std::optional<cv::Rect> get_pre_box(const std::string& task_name) const;
    void set_pre_box(std::string task_name, cv::Rect box);

private:
    std::map<std::string, cv::Rect> pre_boxes_;
};

MAA_NS_END
