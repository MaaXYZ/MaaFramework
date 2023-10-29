#ifdef WITH_THRIFT_CONTROLLER

#include "CustomThriftController.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4245 4706)
#endif
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_NS_BEGIN

CustomThriftController::CustomThriftController(const std::string& param_str, MaaControllerCallback callback,
                                               void* callback_arg)
    : ControllerMgr(callback, callback_arg)
{
    LogDebug << VAR(param_str);

    static const std::unordered_map<std::string, ThriftControllerTypeEnum> type_map = {
        { "Socket", ThriftControllerTypeEnum::MaaThriftControllerType_Socket },
        { "UnixDomainSocket", ThriftControllerTypeEnum::MaaThriftControllerType_UnixDomainSocket },
    };

    using namespace apache::thrift;

    std::shared_ptr<transport::TSocket> socket;

    auto param_json = json::parse(param_str);
    if (!param_json.has_value()) {
        LogError << "Invalid param: " << param_str;
        throw std::runtime_error("MaaThriftController: Invalid param");
    }

    auto type = param_json->at("type").as_string();

    switch (type_map.at(type)) {
    case ThriftControllerTypeEnum::MaaThriftControllerType_Socket:
        if (param_json->at("param").is_object()) {
            auto host = param_json->at("param").at("host").as_string();
            auto port = param_json->at("param").at("port").as_integer();
            socket = std::make_shared<transport::TSocket>(host, port);
        }
        else {
            LogError << "Invalid param: " << param_json->at("param");
            throw std::runtime_error("MaaThriftController: Invalid param");
        }
        break;
    case ThriftControllerTypeEnum::MaaThriftControllerType_UnixDomainSocket:
        if (param_json->at("param").is_string()) {
            auto path = param_json->at("param").as_string();
            socket = std::make_shared<transport::TSocket>(path);
        }
        else {
            LogError << "Invalid param: " << param_json->at("param");
            throw std::runtime_error("MaaThriftController: Invalid param");
        }
        break;
    default:
        LogError << "Unknown type: " << type;
        throw std::runtime_error("MaaThriftController: Unknown type");
    }

    transport_ = std::make_shared<transport::TBufferedTransport>(socket);
    auto protocol = std::make_shared<protocol::TBinaryProtocol>(transport_);

    client_ = std::make_shared<ThriftController::ThriftControllerClient>(protocol);
}

CustomThriftController::~CustomThriftController()
{
    if (transport_) {
        transport_->close();
    }
}

bool CustomThriftController::_connect()
{
    try {
        transport_->open();
    }
    catch (const std::exception& e) {
        LogError << "transport_->open() failed: " << e.what();
        return false;
    }

    return client_->connect();
}

std::string CustomThriftController::_get_uuid() const
{
    LogFunc;

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return {};
    }

    std::string uuid;
    client_->get_uuid(uuid);
    return uuid;
}

std::pair<int, int> CustomThriftController::_get_resolution() const
{
    LogFunc;

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return {};
    }

    ThriftController::Size resolution;
    client_->get_resolution(resolution);
    return { resolution.width, resolution.height };
}

bool CustomThriftController::_click(ClickParam param)
{
    LogFunc << VAR(param.x) << VAR(param.y);

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::ClickParam thrift_param;
    thrift_param.point.x = param.x;
    thrift_param.point.y = param.y;

    return client_->click(thrift_param);
}

bool CustomThriftController::_swipe(SwipeParam param)
{
    LogFunc << VAR(param.x1) << VAR(param.x2) << VAR(param.y1) << VAR(param.y2) << VAR(param.duration);

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }
    ThriftController::SwipeParam thrift_param;
    thrift_param.point1.x = param.x1;
    thrift_param.point1.y = param.y1;
    thrift_param.point2.x = param.x2;
    thrift_param.point2.y = param.y2;
    thrift_param.duration = param.duration;
    return client_->swipe(thrift_param);
}

bool CustomThriftController::_touch_down(TouchParam param)
{
    LogFunc << VAR(param.contact) << VAR(param.x) << VAR(param.y) << VAR(param.pressure);

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::TouchParam thrift_param;
    thrift_param.contact = param.contact;
    thrift_param.point.x = param.x;
    thrift_param.point.y = param.y;
    thrift_param.pressure = param.pressure;

    return client_->touch_down(thrift_param);
}

bool CustomThriftController::_touch_move(TouchParam param)
{
    LogFunc << VAR(param.contact) << VAR(param.x) << VAR(param.y) << VAR(param.pressure);

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::TouchParam thrift_param;
    thrift_param.contact = param.contact;
    thrift_param.point.x = param.x;
    thrift_param.point.y = param.y;
    thrift_param.pressure = param.pressure;

    return client_->touch_move(thrift_param);
}

bool CustomThriftController::_touch_up(TouchParam param)
{
    LogFunc << VAR(param.contact);

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::TouchParam thrift_param;
    thrift_param.contact = param.contact;

    return client_->touch_up(thrift_param);
}

bool CustomThriftController::_press_key(PressKeyParam param)
{
    LogFunc;

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    ThriftController::PressKeyParam thrift_param;
    thrift_param.keycode = param.keycode;

    return client_->press_key(thrift_param);
}

cv::Mat CustomThriftController::_screencap()
{
    LogFunc;

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return {};
    }

    ThriftController::CustomImage img;
    client_->screencap(img);
    if (img.data.empty()) {
        LogError << "client_->screencap() return empty buffer";
        return {};
    }

    cv::Mat orig_mat(img.size.height, img.size.width, img.type, img.data.data());
    cv::Mat out_mat;
    orig_mat.copyTo(out_mat);

    return out_mat;
}

bool CustomThriftController::_start_app(AppParam param)
{
    LogFunc << VAR(param.package);

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    return client_->start_game(param.package);
}

bool CustomThriftController::_stop_app(AppParam param)
{
    LogFunc << VAR(param.package);

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return false;
    }

    return client_->stop_game(param.package);
}

MAA_CTRL_NS_END

#endif // WITH_THRIFT_CONTROLLER
