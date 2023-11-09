#include "ThriftControllerAgent.h"

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_UNIT_NS_BEGIN

ThriftControllerAgent::~ThriftControllerAgent()
{
    LogFunc;

    if (transport_) {
        transport_->close();
    }
}

bool ThriftControllerAgent::find_device(std::vector<std::string>& devices)
{
    std::ignore = devices;

    return false;
}

bool ThriftControllerAgent::connect()
{
    LogFunc;

    if (!client_ || !transport_) {
        LogError << "client_ or transport_ is nullptr or transport_ is not open";
        return false;
    }

    try {
        transport_->open();
    }
    catch (const std::exception& e) {
        LogError << "transport_->open() failed: " << e.what();
        return false;
    }

    return client_->connect();
}

bool ThriftControllerAgent::request_uuid(std::string& uuid)
{
    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    client_->request_uuid(uuid);
    return true;
}

bool ThriftControllerAgent::request_resolution(int& width, int& height)
{
    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::Size resolution;
    client_->request_resolution(resolution);
    width = resolution.width;
    height = resolution.height;
    return true;
}

bool ThriftControllerAgent::start_app(const std::string& intent)
{
    LogFunc << VAR(intent);

    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    return client_->start_app(intent);
}

bool ThriftControllerAgent::stop_app(const std::string& intent)
{
    LogFunc << VAR(intent);

    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    return client_->stop_app(intent);
}

bool ThriftControllerAgent::screencap(cv::Mat& image)
{
    LogFunc;

    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::CustomImage custom_image;
    client_->screencap(custom_image);
    if (custom_image.png_data.empty()) {
        LogError << "custom_image.png_data is empty";
        return false;
    }

    image = cv::imdecode({ custom_image.png_data.data(), static_cast<int>(custom_image.png_data.size()) },
                         cv::IMREAD_COLOR);
    return true;
}

bool ThriftControllerAgent::click(int x, int y)
{
    LogFunc << VAR(x) << VAR(y);

    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::ClickParam thrift_param;
    thrift_param.point.x = x;
    thrift_param.point.y = y;

    return client_->click(thrift_param);
}

bool ThriftControllerAgent::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogFunc << VAR(x1) << VAR(x2) << VAR(y1) << VAR(y2) << VAR(duration);

    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }
    ThriftController::SwipeParam thrift_param;
    thrift_param.point1.x = x1;
    thrift_param.point1.y = y1;
    thrift_param.point2.x = x2;
    thrift_param.point2.y = y2;
    thrift_param.duration = duration;
    return client_->swipe(thrift_param);
}

bool ThriftControllerAgent::touch_down(int contact, int x, int y, int pressure)
{
    LogFunc << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::TouchParam thrift_param;
    thrift_param.contact = contact;
    thrift_param.point.x = x;
    thrift_param.point.y = y;
    thrift_param.pressure = pressure;

    return client_->touch_down(thrift_param);
}

bool ThriftControllerAgent::touch_move(int contact, int x, int y, int pressure)
{
    LogFunc << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::TouchParam thrift_param;
    thrift_param.contact = contact;
    thrift_param.point.x = x;
    thrift_param.point.y = y;
    thrift_param.pressure = pressure;

    return client_->touch_move(thrift_param);
}

bool ThriftControllerAgent::touch_up(int contact)
{
    LogFunc << VAR(contact);

    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::TouchParam thrift_param;
    thrift_param.contact = contact;

    return client_->touch_up(thrift_param);
}

bool ThriftControllerAgent::press_key(int key)
{
    LogFunc << VAR(key);

    if (!client_ || !transport_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::PressKeyParam thrift_param;
    thrift_param.keycode = key;

    return client_->press_key(thrift_param);
}

MAA_CTRL_UNIT_NS_END
