#include "AndroidNativeControlUnitMgr.h"

#include "Input/NativeInput.h"
#include "Screencap/NativeScreencap.h"

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

AndroidNativeControlUnitMgr::AndroidNativeControlUnitMgr()
{
    LogFunc;
}

AndroidNativeControlUnitMgr::~AndroidNativeControlUnitMgr()
{
    LogFunc;
}

bool AndroidNativeControlUnitMgr::connect()
{
    LogFunc;

    screencap_ = std::make_shared<NativeScreencap>();
    if (!screencap_->init()) {
        LogError << "failed to init screencap";
        return false;
    }

    auto resolution = screencap_->get_resolution();
    display_width_ = resolution.first;
    display_height_ = resolution.second;

    LogInfo << "Display resolution:" << display_width_ << "x" << display_height_;

    input_ = std::make_shared<NativeInput>();
    if (!input_->init(display_width_, display_height_)) {
        LogError << "failed to init input";
        return false;
    }

    return true;
}

bool AndroidNativeControlUnitMgr::request_uuid(std::string& uuid)
{
    LogFunc;

    // 使用 Android ID 或其他唯一标识
    uuid = "android_native_device";
    return true;
}

MaaControllerFeature AndroidNativeControlUnitMgr::get_features() const
{
    return MaaControllerFeature_None;
}

bool AndroidNativeControlUnitMgr::start_app(const std::string& intent)
{
    LogFunc << VAR(intent);

    // 通过 JNI 调用 startActivity
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }
    return input_->start_app(intent);
}

bool AndroidNativeControlUnitMgr::stop_app(const std::string& intent)
{
    LogFunc << VAR(intent);

    // 通过 JNI 调用 forceStopPackage
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }
    return input_->stop_app(intent);
}

bool AndroidNativeControlUnitMgr::screencap(cv::Mat& image)
{
    if (!screencap_) {
        LogError << "screencap_ is nullptr";
        return false;
    }

    return screencap_->screencap(image);
}

bool AndroidNativeControlUnitMgr::click(int x, int y)
{
    LogInfo << VAR(x) << VAR(y);

    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->click(x, y);
}

bool AndroidNativeControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->swipe(x1, y1, x2, y2, duration);
}

bool AndroidNativeControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_down(contact, x, y, pressure);
}

bool AndroidNativeControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_move(contact, x, y, pressure);
}

bool AndroidNativeControlUnitMgr::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_up(contact);
}

bool AndroidNativeControlUnitMgr::click_key(int key)
{
    LogInfo << VAR(key);

    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->click_key(key);
}

bool AndroidNativeControlUnitMgr::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->input_text(text);
}

bool AndroidNativeControlUnitMgr::key_down(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->key_down(key);
}

bool AndroidNativeControlUnitMgr::key_up(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->key_up(key);
}

bool AndroidNativeControlUnitMgr::scroll(int dx, int dy)
{
    LogInfo << VAR(dx) << VAR(dy);

    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    // 使用 swipe 模拟滚动
    int cx = display_width_ / 2;
    int cy = display_height_ / 2;

    return input_->swipe(cx, cy, cx + dx, cy + dy, 200);
}

MAA_CTRL_UNIT_NS_END
