#pragma once

#include <atomic>

#include "Base/AsyncRunner.hpp"
#include "Common/MaaTypes.h"
#include "DefaultPipelineMgr.h"
#include "MaaFramework/Instance/MaaResource.h"
#include "OCRResMgr.h"
#include "ONNXResMgr.h"
#include "PipelineResMgr.h"
#include "TemplateResMgr.h"
#include "Utils/EventDispatcher.hpp"

#include "Common/Conf.h"

MAA_RES_NS_BEGIN

enum class PostPathType
{
    Bundle,
    OcrModel,
    Pipeline,
    Image,
};

struct PostPathItem
{
    PostPathType type = PostPathType::Bundle;
    std::filesystem::path path;
};

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
    virtual MaaResId post_ocr_model(const std::filesystem::path& path) override;
    virtual MaaResId post_pipeline(const std::filesystem::path& path) override;
    virtual MaaResId post_image(const std::filesystem::path& path) override;

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

    MaaResId post_path(PostPathType type, const std::filesystem::path& path);

    bool run_load(typename AsyncRunner<PostPathItem>::Id id, PostPathItem item);
    bool load_bundle(const std::filesystem::path& path);
    bool load_ocr_model(const std::filesystem::path& path);
    bool load_pipeline(const std::filesystem::path& path);
    bool load_image(const std::filesystem::path& path);
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

private:
    std::vector<std::filesystem::path> paths_;
    mutable std::string hash_cache_;
    std::atomic_bool valid_ = true;

    std::unique_ptr<AsyncRunner<PostPathItem>> res_loader_ = nullptr;
    EventDispatcher notifier_;

    MaaInferenceDevice inference_device_ = MaaInferenceDevice_Auto;
    MaaInferenceExecutionProvider inference_ep_ = MaaInferenceExecutionProvider_Auto;
    bool inference_device_setted_ = false;
};

MAA_RES_NS_END
