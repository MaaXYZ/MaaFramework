#include "ControllerMgr.h"

MAA_CTRL_NS_BEGIN

ControllerMgr::ControllerMgr(const std::filesystem::path& adb_path, const std::string& address, const Config& config,
                             MaaControllerCallback callback, void* callback_arg)
    : adb_path_(adb_path), address_(address), config_(config), callback_(callback), callback_arg_(callback_arg)
{}

ControllerMgr::~ControllerMgr() {}

MaaControllerAPI* ControllerMgr::create_ex(const std::filesystem::path& adb_path, const std::string& address,
                                           const json::value& config_json, MaaControllerCallback callback,
                                           void* callback_arg)
{
    Config config;
    // TODO: parse json
    return new ControllerMgr(adb_path, address, config, callback, callback_arg);
}

void ControllerMgr::destroy(MaaControllerAPI** handle_ptr)
{
    if (handle_ptr == nullptr || *handle_ptr == nullptr) {
        return;
    }
    delete *handle_ptr;
    *handle_ptr = nullptr;
}

bool ControllerMgr::set_option(ControllerOptionKey key, const std::string& value)
{
    return false;
}

bool ControllerMgr::connecting() const
{
    return false;
}

bool ControllerMgr::connected() const
{
    return false;
}

MaaCtrlId ControllerMgr::click(int x, int y)
{
    return MaaCtrlId();
}

MaaCtrlId ControllerMgr::screencap()
{
    return MaaCtrlId();
}

std::vector<unsigned char> ControllerMgr::get_image() const
{
    return std::vector<unsigned char>();
}

std::string ControllerMgr::get_uuid() const
{
    return std::string();
}

MAA_CTRL_NS_END
