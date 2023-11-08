#pragma once

#include "API/MaaTypes.h"
#include "Base/AsyncRunner.hpp"
#include "Base/MessageNotifier.hpp"
#include "OCRResMgr.h"
#include "ONNXResMgr.h"
#include "PipelineResMgr.h"
#include "TemplateResMgr.h"

#include <atomic>

MAA_RES_NS_BEGIN

class ResourceMgr : public MaaResourceAPI
{
public:
    ResourceMgr(MaaResourceCallback callback, MaaCallbackTransparentArg callback_arg);
    virtual ~ResourceMgr() override;

    virtual bool set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaResId post_path(std::filesystem::path path) override;

    virtual MaaStatus status(MaaResId res_id) const override;
    virtual MaaStatus wait(MaaResId res_id) const override;
    virtual MaaBool loaded() const override;

    virtual std::string get_hash() const override;
    virtual std::vector<std::string> get_task_list() const override;

public:
    const auto& pipeline_res() const { return pipeline_res_; }
    auto& pipeline_res() { return pipeline_res_; }
    const auto& ocr_res() const { return ocr_res_; }
    auto& ocr_res() { return ocr_res_; }
    const auto& onnx_res() const { return onnx_res_; }
    auto& onnx_res() { return onnx_res_; }
    const auto& template_res() const { return template_res_; }
    auto& template_res() { return template_res_; }

private:
    bool run_load(typename AsyncRunner<std::filesystem::path>::Id id, std::filesystem::path path);
    bool load(const std::filesystem::path& path);

private:
    PipelineResMgr pipeline_res_;
    OCRResMgr ocr_res_;
    ONNXResMgr onnx_res_;
    TemplateResMgr template_res_;

private:
    std::vector<std::filesystem::path> paths_;
    mutable std::string hash_cache_;
    std::atomic_bool loaded_ = false;

    std::unique_ptr<AsyncRunner<std::filesystem::path>> res_loader_ = nullptr;
    MessageNotifier<MaaResourceCallback> notifier;
};

MAA_RES_NS_END
