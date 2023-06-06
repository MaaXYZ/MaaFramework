#pragma once

#include "Base/AsyncCallback.hpp"
#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

MAA_RES_NS_BEGIN

class ResourceMgr : public MaaResourceAPI
{
public:
    ResourceMgr(const std::filesystem::path& user_path, MaaResourceCallback callback, void* callback_arg);
    virtual ~ResourceMgr() override;

    virtual bool set_option(std::string_view key, std::string_view value) override;

    virtual MaaResId post_load(const std::filesystem::path& path) override;

    virtual MaaStatus status(MaaResId res_id) const override;
    virtual MaaBool loaded() const override;

    virtual std::string get_hash() const override;

private:
    std::filesystem::path user_path_;
    std::vector<std::filesystem::path> paths_;
    AsyncCallback<MaaResourceCallback, void*> notifier;
};

MAA_RES_NS_END
