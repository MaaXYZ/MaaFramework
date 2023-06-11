#pragma once

#include "Base/AsyncCallback.hpp"
#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

#include <atomic>

MAA_RES_NS_BEGIN

class PipelineConfig;
class Config;

class ResourceMgr : public MaaResourceAPI
{
public:
    ResourceMgr(const std::filesystem::path& user_path, MaaResourceCallback callback, void* callback_arg);
    virtual ~ResourceMgr() override;

    virtual bool set_option(MaaResOption key, const std::string& value) override;

    virtual MaaResId post_resource(std::filesystem::path path) override;

    virtual MaaStatus status(MaaResId res_id) const override;
    virtual MaaStatus wait(MaaResId res_id) const override;
    virtual MaaBool loaded() const override;

    virtual std::string get_hash() const override;

public:
    const auto& adb_cfg() const { return adb_cfg_; }
    const auto& pipeline_cfg() const { return pipeline_cfg_; }

private:
    bool run_load(typename AsyncRunner<std::filesystem::path>::Id id, std::filesystem::path path);
    bool load(const std::filesystem::path& path);

private:
    std::shared_ptr<Config> adb_cfg_ = nullptr;
    std::shared_ptr<PipelineConfig> pipeline_cfg_ = nullptr;

private:
    std::filesystem::path user_path_;
    std::vector<std::filesystem::path> paths_;
    std::atomic_bool loaded_ = false;

    std::unique_ptr<AsyncRunner<std::filesystem::path>> res_loader_ = nullptr;
    AsyncCallback<MaaResourceCallback, void*> notifier;
};

MAA_RES_NS_END
