#pragma once

#include <memory>
#include <meojson/json.hpp>

#include "Common/MaaTypes.h"
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
    static std::shared_ptr<Context> create(MaaTaskId id, Tasker* tasker);
    std::shared_ptr<Context> getptr();
    std::shared_ptr<const Context> getptr() const;

    Context(MaaTaskId id, Tasker* tasker, PrivateArg);
    Context(const Context& other);
    Context(Context&& other) = default;

    virtual ~Context() override = default;

public: // from MaaContextAPI
    virtual MaaTaskId run_task(const std::string& entry, const json::object& pipeline_override) override;
    virtual MaaRecoId run_recognition(const std::string& entry, const json::object& pipeline_override, const cv::Mat& image) override;
    virtual MaaNodeId
        run_action(const std::string& entry, const json::object& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
            override;
    virtual bool override_pipeline(const json::object& pipeline_override) override;
    virtual bool override_next(const std::string& name, const std::vector<std::string>& next) override;
    virtual std::optional<json::object> get_node_data(const std::string& node_name) const override;

    virtual Context* clone() const override;

    virtual MaaTaskId task_id() const override;
    virtual Tasker* tasker() const override;

public:
    std::optional<PipelineData> get_pipeline_data(const std::string& node_name) const;
    bool& need_to_stop();

private:
    bool check_pipeline() const;

    MaaTaskId task_id_ = 0;
    Tasker* tasker_ = nullptr;

    PipelineDataMap pipeline_override_;

private:
    bool need_to_stop_ = false;

    mutable std::vector<std::shared_ptr<Context>> clone_holder_;
};

MAA_TASK_NS_END
