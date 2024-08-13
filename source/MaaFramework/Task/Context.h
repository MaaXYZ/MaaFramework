#pragma once

#include <stack>
#include <string_view>

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "TaskBase.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class Context : public MaaContext
{
public:
    explicit Context(Tasker* tasker, TaskBase& task);
    virtual ~Context() override = default;

public: // from MaaContextAPI
    virtual MaaTaskId run_pipeline(std::string task, std::string_view param) override;
    virtual MaaTaskId run_recognition(std::string task, std::string_view param, cv::Mat image) override;
    virtual MaaTaskId run_action(std::string task, std::string_view param, cv::Rect cur_box, std::string cur_detail) override;

    virtual Tasker* tasker() override;

private:
    Tasker* tasker_ = nullptr;
};

MAA_TASK_NS_END
