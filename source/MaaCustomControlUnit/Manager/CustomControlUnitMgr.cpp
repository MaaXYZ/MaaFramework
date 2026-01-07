#include "CustomControlUnitMgr.h"

#include "MaaUtils/Buffer/ImageBuffer.hpp"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

CustomControlUnitMgr::CustomControlUnitMgr(MaaCustomControllerCallbacks* controller, void* controller_arg)
    : controller_(controller)
    , controller_arg_(controller_arg)
{
}

bool CustomControlUnitMgr::connect()
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->connect);

    if (!controller_ || !controller_->connect) {
        LogError << "controller_ or controller_->connect is nullptr";
        return false;
    }

    return controller_->connect(controller_arg_);
}

bool CustomControlUnitMgr::connected() const
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->connected);

    if (!controller_ || !controller_->connected) {
        // connected 回调是可选的，未提供时默认返回 true（已连接）
        return true;
    }

    return controller_->connected(controller_arg_);
}

bool CustomControlUnitMgr::request_uuid(std::string& uuid)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->request_uuid);

    if (!controller_ || !controller_->request_uuid) {
        LogError << "controller_ or controller_->request_uuid is nullptr";
        return false;
    }

    StringBuffer buffer;
    bool ret = controller_->request_uuid(controller_arg_, &buffer);
    if (!ret) {
        LogError << "failed to request_uuid" << VAR(ret);
        return false;
    }
    uuid = buffer.get();

    return true;
}

MaaControllerFeature CustomControlUnitMgr::get_features() const
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->get_features);

    if (!controller_ || !controller_->get_features) {
        LogError << "controller_ or controller_->get_features is nullptr";
        return MaaControllerFeature_None;
    }

    MaaControllerFeature features = controller_->get_features(controller_arg_);
    return features;
}

bool CustomControlUnitMgr::start_app(const std::string& intent)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->start_app) << VAR(intent);

    if (!controller_ || !controller_->start_app) {
        LogError << "controller_ or controller_->start_app is nullptr";
        return false;
    }

    return controller_->start_app(intent.c_str(), controller_arg_);
}

bool CustomControlUnitMgr::stop_app(const std::string& intent)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->stop_app) << VAR(intent);

    if (!controller_ || !controller_->stop_app) {
        LogError << "controller_ or controller_->stop_app is nullptr";
        return false;
    }

    return controller_->stop_app(intent.c_str(), controller_arg_);
}

bool CustomControlUnitMgr::screencap(cv::Mat& image)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->screencap);

    if (!controller_ || !controller_->screencap) {
        LogError << "controller_ or controller_->screencap is nullptr";
        return false;
    }

    ImageBuffer buffer;
    bool ret = controller_->screencap(controller_arg_, &buffer);
    if (!ret) {
        LogError << "failed to cached_image" << VAR(ret);
        return false;
    }

    image = buffer.get();
    return true;
}

bool CustomControlUnitMgr::click(int x, int y)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->click) << VAR(x) << VAR(y);

    if (!controller_ || !controller_->click) {
        LogError << "controller_ or controller_->click is nullptr";
        return false;
    }

    return controller_->click(x, y, controller_arg_);
}

bool CustomControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->swipe) << VAR(x1) << VAR(x2) << VAR(y1) << VAR(y2) << VAR(duration);

    if (!controller_ || !controller_->swipe) {
        LogError << "controller_ or controller_->swipe is nullptr";
        return false;
    }
    return controller_->swipe(x1, y1, x2, y2, duration, controller_arg_);
}

bool CustomControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->touch_down) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!controller_ || !controller_->touch_down) {
        LogError << "controller_ or controller_->touch_down is nullptr";
        return false;
    }

    return controller_->touch_down(contact, x, y, pressure, controller_arg_);
}

bool CustomControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->touch_move) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!controller_ || !controller_->touch_move) {
        LogError << "controller_ or controller_->touch_move is nullptr";
        return false;
    }

    return controller_->touch_move(contact, x, y, pressure, controller_arg_);
}

bool CustomControlUnitMgr::touch_up(int contact)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->touch_up) << VAR(contact);

    if (!controller_ || !controller_->touch_up) {
        LogError << "controller_ or controller_->touch_up is nullptr";
        return false;
    }

    return controller_->touch_up(contact, controller_arg_);
}

bool CustomControlUnitMgr::click_key(int key)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->click_key) << VAR(key);

    if (!controller_ || !controller_->click_key) {
        LogError << "controller_ or controller_->click_key is nullptr";
        return false;
    }

    return controller_->click_key(key, controller_arg_);
}

bool CustomControlUnitMgr::input_text(const std::string& text)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->input_text) << VAR(text);

    if (!controller_ || !controller_->input_text) {
        LogError << "controller_ or controller_->input_text is nullptr";
        return false;
    }

    return controller_->input_text(text.c_str(), controller_arg_);
}

bool CustomControlUnitMgr::key_down(int key)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->key_down) << VAR(key);

    if (!controller_ || !controller_->key_down) {
        LogError << "controller_ or controller_->key_down is nullptr";
        return false;
    }

    return controller_->key_down(key, controller_arg_);
}

bool CustomControlUnitMgr::key_up(int key)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->key_up) << VAR(key);

    if (!controller_ || !controller_->key_up) {
        LogError << "controller_ or controller_->key_up is nullptr";
        return false;
    }

    return controller_->key_up(key, controller_arg_);
}

bool CustomControlUnitMgr::scroll(int dx, int dy)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->scroll) << VAR(dx) << VAR(dy);

    if (!controller_ || !controller_->scroll) {
        LogError << "controller_ or controller_->scroll is nullptr";
        return false;
    }

    return controller_->scroll(dx, dy, controller_arg_);
}

MAA_CTRL_UNIT_NS_END
