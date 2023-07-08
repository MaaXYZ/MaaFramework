#pragma once

#include "Base/AsyncRunner.hpp"
#include "Base/MessageNotifier.hpp"
#include "Common/MaaTypes.h"
#include "OCRConfig.h"
#include "PipelineConfig.h"
#include "TemplateConfig.h"

#include <atomic>

MAA_RES_NS_BEGIN

class ResourceMgr : public MaaResourceAPI
{
public:
    ResourceMgr(const std::filesystem::path& user_path, MaaResourceCallback callback,
                MaaCallbackTransparentArg callback_arg);
    virtual ~ResourceMgr() override;

    virtual bool set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size) override;

    virtual MaaResId post_resource(std::filesystem::path path) override;

    virtual MaaStatus status(MaaResId res_id) const override;
    virtual MaaStatus wait(MaaResId res_id) const override;
    virtual MaaBool loaded() const override;

    virtual std::string get_hash() const override;

public:
    const auto& pipeline_cfg() const { return pipeline_cfg_; }
    // const auto& template_cfg() const { return template_cfg_; }
    const auto& ocr_cfg() const { return ocr_cfg_; }

private:
    bool run_load(typename AsyncRunner<std::filesystem::path>::Id id, std::filesystem::path path);
    bool load(const std::filesystem::path& path);

private:
    PipelineConfig pipeline_cfg_;
    // TemplateConfig template_cfg_;
    OCRConfig ocr_cfg_;

private:
    std::filesystem::path user_path_;
    std::vector<std::filesystem::path> paths_;
    std::atomic_bool loaded_ = false;

    std::unique_ptr<AsyncRunner<std::filesystem::path>> res_loader_ = nullptr;
    MessageNotifier<MaaResourceCallback> notifier;
};

MAA_RES_NS_END
