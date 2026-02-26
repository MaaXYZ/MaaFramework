#include "MacOSControlUnitMgr.h"

#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/Logger.h"

#include "Input/GlobalEventInput.h"
#include "Screencap/ScreenCaptureKitScreencap.h"

MAA_CTRL_UNIT_NS_BEGIN

MacOSControlUnitMgr::MacOSControlUnitMgr(uint32_t window_id, MaaMacOSScreencapMethod screencap_method, MaaMacOSInputMethod input_method)
    : window_id_(window_id)
    , screencap_method_(screencap_method)
    , input_method_(input_method)
{
}

bool MacOSControlUnitMgr::connect()
{
    connected_ = false;

    switch (screencap_method_) {
    case MaaMacOSScreencapMethod_ScreenCaptureKit:
        // 检查macOS版本
        if (__builtin_available(macOS 14.0, *)) {
            screencap_ = std::make_shared<ScreenCaptureKitScreencap>(window_id_);
        }
        else {
            LogError << "macOS 14.0 or later required for ScreenCaptureKit";
            return false;
        }
        break;
    case MaaMacOSScreencapMethod_None:
        LogWarn << "No screencap method specified, screencap will not work";
        break;
    default:
        LogError << "Unknown screencap method: " << static_cast<int>(screencap_method_);
        break;
    }

    switch (input_method_) {
    case MaaMacOSInputMethod_GlobalEvent:
        input_ = std::make_shared<GlobalEventInput>(window_id_);
        break;
    case MaaMacOSInputMethod_None:
        LogWarn << "No input method specified, input will not work";
        break;
    default:
        LogError << "Unknown input method: " << static_cast<int>(input_method_);
        break;
    }

    connected_ = true;
    return true;
}

bool MacOSControlUnitMgr::connected() const
{
    return connected_;
}

bool MacOSControlUnitMgr::request_uuid(std::string& uuid)
{
    uuid = std::to_string(window_id_);
    return true;
}

MaaControllerFeature MacOSControlUnitMgr::get_features() const
{
    return MaaControllerFeature_None;
}

bool MacOSControlUnitMgr::start_app(const std::string& intent)
{
    (void)intent;
    LogWarn << "start_app not supported on macOS controller";
    return false;
}

bool MacOSControlUnitMgr::stop_app(const std::string& intent)
{
    (void)intent;
    LogWarn << "stop_app not supported on macOS controller";
    return false;
}

bool MacOSControlUnitMgr::screencap(cv::Mat& image)
{
    if (!screencap_) {
        LogError << "screencap_ is nullptr";
        return false;
    }

    auto opt = screencap_->screencap();
    if (!opt) {
        LogError << "screencap failed";
        return false;
    }

    image = std::move(*opt);
    return true;
}

bool MacOSControlUnitMgr::click(int x, int y)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->click(x, y);
}

bool MacOSControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->swipe(x1, y1, x2, y2, duration);
}

bool MacOSControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_down(contact, x, y, pressure);
}

bool MacOSControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_move(contact, x, y, pressure);
}

bool MacOSControlUnitMgr::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_up(contact);
}

bool MacOSControlUnitMgr::click_key(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->click_key(key);
}

bool MacOSControlUnitMgr::input_text(const std::string& text)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->input_text(text);
}

bool MacOSControlUnitMgr::key_down(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->key_down(key);
}

bool MacOSControlUnitMgr::key_up(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->key_up(key);
}

bool MacOSControlUnitMgr::scroll(int dx, int dy)
{
    (void)dx;
    (void)dy;
    LogWarn << "scroll not supported on macOS controller";
    return false;
}

MAA_CTRL_UNIT_NS_END
