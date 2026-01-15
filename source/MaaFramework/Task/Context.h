#pragma once

#include <memory>
#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "Common/MaaTypes.h"
#include "MaaFramework/MaaDef.h"
#include "Resource/PipelineTypes.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

struct TaskState
{
    std::unordered_map<std::string, size_t> hit_count;
    std::unordered_map<std::string, std::string> anchors;
};

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
    std::shared_ptr<Context> make_clone() const;

    Context(MaaTaskId id, Tasker* tasker, PrivateArg);
    Context(const Context& other);
    Context(Context&& other) = default;

    virtual ~Context() override = default;

public: // from MaaContextAPI
    virtual MaaTaskId run_task(const std::string& entry, const json::value& pipeline_override) override;
    virtual MaaRecoId run_recognition(const std::string& entry, const json::value& pipeline_override, const cv::Mat& image) override;
    virtual MaaActId
        run_action(const std::string& entry, const json::value& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
            override;
    virtual bool override_pipeline(const json::value& pipeline_override) override;
    virtual bool override_next(const std::string& node_name, const std::vector<std::string>& next) override;
    virtual bool override_image(const std::string& image_name, const cv::Mat& image) override;
    virtual std::optional<json::object> get_node_data(const std::string& node_name) const override;

    virtual Context* clone() const override;

    virtual MaaTaskId task_id() const override;
    virtual Tasker* tasker() const override;

public:
    std::optional<PipelineData> get_pipeline_data(const std::string& node_name) const;
    std::optional<PipelineData> get_pipeline_data(const MAA_RES_NS::NodeAttr& node_attr) const;
    std::vector<cv::Mat> get_images(const std::vector<std::string>& names);

    bool& need_to_stop();

    virtual size_t get_hit_count(const std::string& node_name) const override;
    void increment_hit_count(const std::string& node_name);
    virtual void clear_hit_count(const std::string& node_name) override;

    virtual void set_anchor(const std::string& anchor_name, const std::string& node_name) override;
    virtual std::optional<std::string> get_anchor(const std::string& anchor_name) const override;

private:
    bool override_pipeline_once(const json::object& pipeline_override, const MAA_RES_NS::DefaultPipelineMgr& default_mgr);
    bool check_pipeline() const;

    MaaTaskId task_id_ = 0;
    Tasker* tasker_ = nullptr;

    // context level
    PipelineDataMap pipeline_override_;
    std::unordered_map<std::string, cv::Mat> image_override_;

    // task level
    std::shared_ptr<TaskState> task_state_ = nullptr;

private:
    bool need_to_stop_ = false;

    mutable std::vector<std::shared_ptr<Context>> clone_holder_;
};

MAA_TASK_NS_END
