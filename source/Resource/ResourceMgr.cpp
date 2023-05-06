#include "ResourceMgr.h"

MAA_RES_NS_BEGIN

ResourceMgr::ResourceMgr(const std::filesystem::path& path, MaaResourceCallback callback, void* callback_arg)
    : path_(path), callback_(callback), callback_arg_(callback_arg)
{}

ResourceMgr::~ResourceMgr() {}

MaaResourceAPI* ResourceMgr::create_ex(const std::filesystem::path& path, MaaResourceCallback callback,
                                       void* callback_arg)
{
    return new ResourceMgr(path, callback, callback_arg);
}

void ResourceMgr::destroy(MaaResourceAPI** handle_ptr)
{
    if (handle_ptr == nullptr || *handle_ptr == nullptr) {
        return;
    }
    delete *handle_ptr;
    *handle_ptr = nullptr;
}

bool ResourceMgr::set_option(ResourceOptionKey key, const std::string& value)
{
    return false;
}

bool ResourceMgr::incremental_load(const std::filesystem::path& path)
{
    return false;
}

bool ResourceMgr::loading() const
{
    return false;
}

bool ResourceMgr::loaded() const
{
    return false;
}

std::string ResourceMgr::get_hash() const
{
    return std::string();
}

MAA_RES_NS_END
