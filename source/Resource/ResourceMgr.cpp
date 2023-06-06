#include "ResourceMgr.h"

MAA_RES_NS_BEGIN

ResourceMgr::ResourceMgr(const std::filesystem::path& user_path, MaaResourceCallback callback, void* callback_arg)
    : user_path_(user_path), notifier(callback, callback_arg)
{}

ResourceMgr::~ResourceMgr() {}

bool ResourceMgr::set_option(std::string_view key, std::string_view value)
{
    return true;
}

MaaResId ResourceMgr::post_load(const std::filesystem::path& path)
{
    return MaaResId();
}

MaaStatus ResourceMgr::status(MaaResId res_id) const
{
    return MaaStatus();
}

MaaBool ResourceMgr::loaded() const
{
    return MaaBool();
}

std::string ResourceMgr::get_hash() const
{
    return std::string();
}

MAA_RES_NS_END
