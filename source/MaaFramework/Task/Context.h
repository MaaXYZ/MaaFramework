#pragma once

#include <memory>
#include <meojson/json.hpp>

#include "API/MaaTypes.h"
#include "Conf/Conf.h"
#include "Resource/PipelineTypes.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

class Context
    : public MaaContext
    , public std::enable_shared_from_this<Context>
{
private:
    struct PrivateArg
    {
    };

public:
    using PipelineData = MAA_RES_NS::PipelineData;
    using PipelineDataMap = MAA_RES_NS::PipelineResMgr::PipelineDataMap;

public:
    static std::shared_ptr<Context> create(MaaTaskId id, Tasker* tasker);
    std::shared_ptr<Context> getptr();
    std::shared_ptr<const Context> getptr() const;

    Context(MaaTaskId id, Tasker* tasker, PrivateArg);
    Context(const Context& other);
    Context(Context&& other) = default;

    virtual ~Context() override = default;

public: // from MaaContextAPI
    virtual MaaTaskId run_pipeline(const std::string& entry, const json::value& pipeline_override) override;
    virtual MaaRecoId run_recognition(const std::string& entry, const json::value& pipeline_override, const cv::Mat& image) override;
    virtual MaaNodeId
        run_action(const std::string& entry, const json::value& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
            override;
    virtual bool override_pipeline(const json::value& pipeline_override) override;

    virtual Context* clone() const override;

    virtual MaaTaskId task_id() const override;
    virtual Tasker* tasker() const override;

public:
    PipelineData get_pipeline_data(const std::string& task_name);

    auto& action_times() { return action_times_map_; }

private:
    Tasker* tasker_ = nullptr;
    MaaTaskId task_id_ = 0;

    PipelineDataMap pipeline_override_;
    std::map<std::string, uint64_t> action_times_map_;

private:
    mutable std::vector<std::shared_ptr<Context>> clone_holder_;
};

MAA_TASK_NS_END
