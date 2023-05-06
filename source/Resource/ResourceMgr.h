#pragma once

#include "Common/MaaMsg.h"
#include "Common/MaaTypes.h"

MAA_RES_NS_BEGIN

class ResourceMgr : public MaaResourceAPI
{
public:
    virtual ~ResourceMgr();

    virtual MaaResourceAPI* create_ex(const std::filesystem::path& path, MaaResourceCallback callback,
                                      void* callback_arg) override;
    virtual void destroy(MaaResourceAPI** handle_ptr) override;

    virtual bool set_option(ResourceOptionKey key, const std::string& value) override;

    virtual bool incremental_load(const std::filesystem::path& path) override;
    virtual bool loading() const override;
    virtual bool loaded() const override;

    virtual std::string get_hash() const override;

protected:
    ResourceMgr(const std::filesystem::path& path, MaaResourceCallback callback, void* callback_arg);

protected:
    std::filesystem::path path_;
    MaaResourceCallback callback_ = nullptr;
    void* callback_arg_ = nullptr;
};

MAA_RES_NS_END
