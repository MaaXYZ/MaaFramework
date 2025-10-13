#include "MacControlUnitMgr.h"

#include "MaaFramework/MaaMsg.h"
#include "Manager/InputAgent.h"
#include "Manager/ScreencapAgent.h"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

MacControlUnitMgr::MacControlUnitMgr(uint32_t windowId, MaaMacScreencapMethod screencap_method, MaaMacInputMethod input_method)
    : window_id_(windowId)
    , screencap_method_(screencap_method)
    , input_method_(input_method)
{
}

bool MacControlUnitMgr::connect()
{
    // TODO: check window id

    if (screencap_method_ != MaaMacScreencapMethod_None) {
        screencap_ = std::make_shared<ScreencapAgent>(screencap_method_, window_id_);
    }
    else {
        LogWarn << "screencap_method_ is None";
    }

    if (input_method_ != MaaMacInputMethod_None) {
        input_ = std::make_shared<InputAgent>(input_method_, window_id_);
    }
    else {
        LogWarn << "input_method_ is None";
    }

    return true;
}

bool MacControlUnitMgr::request_uuid(std::string& uuid)
{
    if (!window_id_) {
        LogWarn << "hwnd_ is nullptr";
    }

    std::stringstream ss;
    ss << window_id_;
    uuid = std::move(ss).str();

    return true;
}

bool MacControlUnitMgr::start_app(const std::string& intent)
{
    // TODO
    std::ignore = intent;

    return false;
}

bool MacControlUnitMgr::stop_app(const std::string& intent)
{
    // TODO
    std::ignore = intent;

    return false;
}

bool MacControlUnitMgr::screencap(cv::Mat& image)
{
    if (!screencap_) {
        LogError << "screencap_ is null";
        return false;
    }

    auto opt = screencap_->screencap();
    if (!opt) {
        LogError << "failed to screencap";
        return false;
    }

    image = std::move(opt).value();

    return true;
}

bool MacControlUnitMgr::click(int x, int y)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->click(x, y);
}

bool MacControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->swipe(x1, y1, x2, y2, duration);
}

bool MacControlUnitMgr::is_touch_availabled() const
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->is_touch_availabled();
}

bool MacControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_down(contact, x, y, pressure);
}

bool MacControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_move(contact, x, y, pressure);
}

bool MacControlUnitMgr::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_up(contact);
}

bool MacControlUnitMgr::click_key(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->click_key(key);
}

bool MacControlUnitMgr::input_text(const std::string& text)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->input_text(text);
}

bool MacControlUnitMgr::is_key_down_up_availabled() const
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->is_key_down_up_availabled();
}

bool MacControlUnitMgr::key_down(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->key_down(key);
}

bool MacControlUnitMgr::key_up(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->key_up(key);
}

MAA_CTRL_UNIT_NS_END
