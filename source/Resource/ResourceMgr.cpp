#include "ResourceMgr.h"

MAA_RES_NS_BEGIN

ResourceMgr::ResourceMgr(const std::filesystem::path& path, const std::filesystem::path& user_path,
                         MaaResourceCallback callback, void* callback_arg)
    : path_(path), user_path_(user_path), callback_(callback), callback_arg_(callback_arg)
{}

ResourceMgr::~ResourceMgr() {}

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
