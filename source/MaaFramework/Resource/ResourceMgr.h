#pragma once

#include <atomic>

#include "API/MaaTypes.h"
#include "Base/AsyncRunner.hpp"
#include "DefaultPipelineMgr.h"
#include "MaaFramework/Instance/MaaResource.h"
#include "OCRResMgr.h"
#include "ONNXResMgr.h"
#include "PipelineResMgr.h"
#include "TemplateResMgr.h"
#include "Utils/MessageNotifier.hpp"

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
    ResourceMgr(MaaNotificationCallback notify, void* notify_trans_arg);
    virtual ~ResourceMgr() override;

public: // MaaResource
    virtual bool set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaResId post_path(const std::filesystem::path& path) override;

    virtual MaaStatus status(MaaResId res_id) const override;
    virtual MaaStatus wait(MaaResId res_id) const override;
    virtual MaaBool valid() const override;
    virtual MaaBool running() const override;
    virtual MaaBool clear() override;

    virtual void register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg) override;
    virtual void unregister_custom_recognition(const std::string& name) override;
    virtual void clear_custom_recognition() override;
    virtual void register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg) override;
    virtual void unregister_custom_action(const std::string& name) override;
    virtual void clear_custom_action() override;

    virtual std::string get_hash() const override;
    virtual std::vector<std::string> get_task_list() const override;

public:
    void post_stop();

    const auto& pipeline_res() const { return pipeline_res_; }

    auto& pipeline_res() { return pipeline_res_; }

    const auto& ocr_res() const { return ocr_res_; }

    auto& ocr_res() { return ocr_res_; }

    const auto& onnx_res() const { return onnx_res_; }

    auto& onnx_res() { return onnx_res_; }

    const auto& template_res() const { return template_res_; }

    auto& template_res() { return template_res_; }

    const auto& default_pipeline() const { return default_pipeline_; }

    CustomRecognitionSession custom_recognition(const std::string& name) const;
    CustomActionSession custom_action(const std::string& name) const;

private:
    bool set_gpu_id(MaaOptionValue value, MaaOptionValueSize val_size);

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

private:
    std::vector<std::filesystem::path> paths_;
    mutable std::string hash_cache_;
    std::atomic_bool valid_ = true;

    std::unique_ptr<AsyncRunner<std::filesystem::path>> res_loader_ = nullptr;
    MessageNotifier notifier;
};

MAA_RES_NS_END
