#include "GamescopeControlUnitMgr.h"

#include <filesystem>
#include <system_error>

#include "Client/EiInput.h"
#include "Client/PipeWireScreencap.h"
#include "Common/VkToEvdev.h"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

GamescopeControlUnitMgr::GamescopeControlUnitMgr(uint32_t node_id, std::string eis_socket_path, bool use_win32_vk_code)
    : screencap_(std::make_unique<PipeWireScreencap>(node_id))
    , input_(std::make_unique<EiInput>(eis_socket_path))
    , eis_socket_path_(std::move(eis_socket_path))
    , node_id_(node_id)
    , use_win32_vk_code_(use_win32_vk_code)
{
    LogFunc << VAR(node_id_) << VAR(eis_socket_path_) << VAR(use_win32_vk_code_);
}

GamescopeControlUnitMgr::~GamescopeControlUnitMgr()
{
    LogFunc;
}

bool GamescopeControlUnitMgr::connect()
{
    if (!screencap_->init()) {
        LogError << "PipeWire screencap init failed";
        return false;
    }
    if (!input_->init()) {
        LogError << "EiInput init failed";
        return false;
    }
    return true;
}

bool GamescopeControlUnitMgr::connected() const
{
    return screencap_ && screencap_->connected() && input_ && input_->connected();
}

bool GamescopeControlUnitMgr::request_uuid(std::string& uuid)
{
    std::error_code ec;
    auto mtime = std::filesystem::last_write_time(eis_socket_path_, ec);
    uuid = std::format("gamescope-{}-{}", node_id_, ec ? 0 : mtime.time_since_epoch().count());
    return true;
}

MaaControllerFeature GamescopeControlUnitMgr::get_features() const
{
    return MaaControllerFeature(
        MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick);
}

bool GamescopeControlUnitMgr::start_app(const std::string& /*intent*/)
{
    return false;
}

bool GamescopeControlUnitMgr::stop_app(const std::string& /*intent*/)
{
    return false;
}

bool GamescopeControlUnitMgr::screencap(cv::Mat& image)
{
    if (!screencap_) {
        LogError << "screencap_ is null";
        return false;
    }
    return screencap_->screencap(image);
}

int GamescopeControlUnitMgr::translate_key(int key) const
{
    if (!use_win32_vk_code_) {
        return key;
    }
    int ev = vk_to_evdev(key);
    if (ev == 0) {
        LogWarn << "No evdev mapping for VK" << VAR(key);
    }
    return ev;
}

bool GamescopeControlUnitMgr::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool GamescopeControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool GamescopeControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }
    return input_->pointer(EiInput::EventPhase::Began, x, y, contact);
}

bool GamescopeControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }
    return input_->pointer(EiInput::EventPhase::Moved, x, y, contact);
}

bool GamescopeControlUnitMgr::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }
    return input_->pointer(EiInput::EventPhase::Ended, 0, 0, contact);
}

bool GamescopeControlUnitMgr::click_key(int key)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(key);
    return false;
}

bool GamescopeControlUnitMgr::input_text(const std::string& text)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }
    return input_->text_utf8(text);
}

bool GamescopeControlUnitMgr::key_down(int key)
{
    int ev = translate_key(key);
    if (!ev) {
        return false;
    }
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }
    return input_->keyboard_key(EiInput::EventPhase::Began, ev);
}

bool GamescopeControlUnitMgr::key_up(int key)
{
    int ev = translate_key(key);
    if (!ev) {
        return false;
    }
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }
    return input_->keyboard_key(EiInput::EventPhase::Ended, ev);
}

bool GamescopeControlUnitMgr::relative_move(int dx, int dy)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }
    return input_->relative_move(dx, dy);
}

bool GamescopeControlUnitMgr::scroll(int dx, int dy)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }
    return input_->scroll(dx, dy);
}

bool GamescopeControlUnitMgr::inactive()
{
    return true;
}

json::object GamescopeControlUnitMgr::get_info() const
{
    return {
        { "type", "gamescope" },
        { "node_id", static_cast<uint64_t>(node_id_) },
        { "eis_socket_path", eis_socket_path_ },
        { "use_win32_vk_code", use_win32_vk_code_ },
    };
}

MAA_CTRL_UNIT_NS_END
