#pragma once

#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Resource/PipelineTypes.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class Context : public MaaContext
{
public:
    using PipelineData = MAA_RES_NS::PipelineData;
    using PipelineDataMap = MAA_RES_NS::PipelineResMgr::PipelineDataMap;

public:
    Context(MaaTaskId id, Tasker* tasker, PipelineDataMap pp_override);
    virtual ~Context() override = default;

public: // from MaaContextAPI
    virtual MaaTaskId run_pipeline(const std::string& entry, const json::value& pipeline_override) override;
    virtual MaaTaskId run_recognition(const std::string& entry, const json::value& pipeline_override, const cv::Mat& image) override;
    virtual MaaTaskId
        run_action(const std::string& entry, const json::value& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
            override;
    virtual bool override_pipeline(const json::value& pipeline_override) override;

    virtual MaaTaskId task_id() const override;

    virtual Tasker* tasker() override;

public:
    PipelineData get_pipeline_data(const std::string& task_name);

private:
    Tasker* tasker_ = nullptr;
    MaaTaskId task_id_ = 0;
    PipelineDataMap pipeline_override_;
};

MAA_TASK_NS_END
