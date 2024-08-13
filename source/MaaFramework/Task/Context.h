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
    explicit Context(TaskBase& task);
    virtual ~Context() override = default;

public: // from MaaContextAPI
    virtual MaaTaskId run_pipeline(std::string task, std::string_view param) override;
    virtual MaaTaskId run_recognition(std::string task, std::string_view param, cv::Mat image) override;
    virtual MaaTaskId run_action(std::string task, std::string_view param, cv::Rect cur_box, std::string cur_detail) override;
    virtual MaaTaskId task_id() const override;

    virtual Tasker* tasker() override;

private:
    const TaskBase& task_;
};

MAA_TASK_NS_END
