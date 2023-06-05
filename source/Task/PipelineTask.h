#pragma once

#include "AbstractTask.h"

MAA_TASK_NS_BEGIN

class PipelineTask : public AbstractTask
{
public:
    virtual ~PipelineTask() override = default;

    virtual bool run() override;

    virtual std::string_view info() const override { return "PipelineTask"; }

protected:
};

MAA_TASK_NS_END
