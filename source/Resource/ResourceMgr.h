#pragma once

#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

MAA_RES_NS_BEGIN

class ResourceMgr : public MaaResourceAPI
{
public:
    ResourceMgr(const std::filesystem::path& path, const std::filesystem::path& user_path, MaaResourceCallback callback,
                void* callback_arg);
    virtual ~ResourceMgr() override;

    virtual bool set_option(std::string_view key, std::string_view value) override;

    virtual bool incremental_load(const std::filesystem::path& path) override;
    virtual bool loading() const override;
    virtual bool loaded() const override;

    virtual std::string get_hash() const override;

protected:
    std::filesystem::path path_;
    std::filesystem::path user_path_;
    MaaResourceCallback callback_ = nullptr;
    void* callback_arg_ = nullptr;
};

MAA_RES_NS_END
