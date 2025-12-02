#pragma once

#include <atomic>
#include <shared_mutex>

#include "Base/AsyncRunner.hpp"
#include "Common/MaaTypes.h"
#include "DefaultPipelineMgr.h"
#include "MaaFramework/Instance/MaaResource.h"
#include "OCRResMgr.h"
#include "ONNXResMgr.h"
#include "PipelineResMgr.h"
#include "RecoCache.h"
#include "RecoJob.h"
#include "TemplateResMgr.h"
#include "Utils/EventDispatcher.hpp"

#include "Common/Conf.h"

MAA_RES_NS_BEGIN

struct CustomRecognitionSession
{
    MaaCustomRecognitionCallback recognition = nullptr;
    void* trans_arg = nullptr;
};

struct CustomActionSession
{
    MaaCustomActionCallback action = nullptr;
    void* trans_arg = nullptr;
};

class ResourceMgr : public MaaResource
{
public:
    ResourceMgr();
    virtual ~ResourceMgr() override;

public: // MaaResource
    virtual bool set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaResId post_bundle(const std::filesystem::path& path) override;

    virtual MaaStatus status(MaaResId res_id) const override;
    virtual MaaStatus wait(MaaResId res_id) const override;
    virtual bool valid() const override;
    virtual bool running() const override;
    virtual bool clear() override;

    virtual bool override_pipeline(const json::value& pipeline_override) override;
    virtual bool override_next(const std::string& node_name, const std::vector<std::string>& next) override;
    virtual bool override_image(const std::string& image_name, const cv::Mat& image) override;
    virtual std::optional<json::object> get_node_data(const std::string& node_name) const override;

    virtual void register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg) override;
    virtual void unregister_custom_recognition(const std::string& name) override;
    virtual void clear_custom_recognition() override;
    virtual void register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg) override;
    virtual void unregister_custom_action(const std::string& name) override;
    virtual void clear_custom_action() override;

    virtual std::string get_hash() const override;
    virtual std::vector<std::string> get_node_list() const override;
    virtual std::vector<std::string> get_custom_recognition_list() const override;
    virtual std::vector<std::string> get_custom_action_list() const override;

    virtual std::optional<MAA_TASK_NS::RecoResult> get_reco_result(MaaRecoId reco_id) const override;
    virtual void clear_reco_cache() override;

    virtual MaaSinkId add_sink(MaaEventCallback callback, void* trans_arg) override;
    virtual void remove_sink(MaaSinkId sink_id) override;
    virtual void clear_sinks() override;

public:
    void post_stop();
    std::string calc_hash();

    const auto& pipeline_res() const { return pipeline_res_; }

    auto& pipeline_res() { return pipeline_res_; }

    const auto& ocr_res() const { return ocr_res_; }

    auto& ocr_res() { return ocr_res_; }

    const auto& onnx_res() const { return onnx_res_; }

    auto& onnx_res() { return onnx_res_; }

    const auto& template_res() const { return template_res_; }

    auto& template_res() { return template_res_; }

    const auto& default_pipeline() const { return default_pipeline_; }

    const std::vector<std::filesystem::path>& paths() const { return paths_; }

    CustomRecognitionSession custom_recognition(const std::string& name) const;
    CustomActionSession custom_action(const std::string& name) const;

    RecoCache& reco_cache() { return reco_cache_; }

    const RecoCache& reco_cache() const { return reco_cache_; }

    virtual MaaRecoId post_recognition(const cv::Mat& image, const PipelineData& pipeline_data) override;
    MaaRecoId post_recognition(const cv::Mat& image, const PipelineData& pipeline_data, std::shared_ptr<MAA_TASK_NS::Context> context);
    virtual std::optional<MAA_TASK_NS::RecoResult> recognition_wait(MaaRecoId reco_id) override;
    static MaaRecoId generate_reco_id();

private:
    bool run_recognition(typename AsyncRunner<RecoJob>::Id id, RecoJob job);

private:
    static const std::unordered_set<MaaInferenceExecutionProvider>& available_providers();

    bool set_inference_device(MaaOptionValue value, MaaOptionValueSize val_size);
    bool set_inference_execution_provider(MaaOptionValue value, MaaOptionValueSize val_size);

    bool check_and_set_inference_device();
    bool use_auto_ep();
    bool use_cpu();
    bool use_directml();
    bool use_coreml();
    bool use_cuda();

    bool run_load(typename AsyncRunner<std::filesystem::path>::Id id, std::filesystem::path path);
    bool load(const std::filesystem::path& path);
    bool check_stop();

private:
    bool need_to_stop_ = false;

private:
    DefaultPipelineMgr default_pipeline_;
    PipelineResMgr pipeline_res_;
    OCRResMgr ocr_res_;
    ONNXResMgr onnx_res_;
    TemplateResMgr template_res_;

    std::unordered_map<std::string, CustomRecognitionSession> custom_recognition_sessions_;
    std::unordered_map<std::string, CustomActionSession> custom_action_sessions_;

    RecoCache reco_cache_;

private:
    std::vector<std::filesystem::path> paths_;
    mutable std::string hash_cache_;
    std::atomic_bool valid_ = true;

    std::unique_ptr<AsyncRunner<std::filesystem::path>> res_loader_ = nullptr;
    std::unique_ptr<AsyncRunner<RecoJob>> recognition_runner_ = nullptr;
    std::map<MaaRecoId, typename AsyncRunner<RecoJob>::Id> reco_id_to_runner_id_;
    mutable std::shared_mutex reco_id_mutex_;
    inline static std::atomic<MaaRecoId> s_reco_id_ = 300'000'000;
    EventDispatcher notifier_;

    MaaInferenceDevice inference_device_ = MaaInferenceDevice_Auto;
    MaaInferenceExecutionProvider inference_ep_ = MaaInferenceExecutionProvider_Auto;
    bool inference_device_setted_ = false;
};

MAA_RES_NS_END
