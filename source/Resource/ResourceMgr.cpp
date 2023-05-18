#include "ResourceMgr.h"

MAA_RES_NS_BEGIN

ResourceMgr::ResourceMgr(const std::filesystem::path& path, const std::filesystem::path& user_path,
                         MaaResourceCallback callback, void* callback_arg)
    : path_(path), user_path_(user_path), callback_(callback), callback_arg_(callback_arg)
{}

ResourceMgr::~ResourceMgr() {}

bool ResourceMgr::set_option(std::string_view key, std::string_view value)
{
    std::ignore = key;
    std::ignore = value;

    return true;
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
