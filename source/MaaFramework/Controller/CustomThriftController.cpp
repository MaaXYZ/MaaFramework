#ifdef WITH_THRIFT

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

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"

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

std::string CustomThriftController::get_uuid() const
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

void CustomThriftController::_click(ClickParam param)
{
    LogFunc << VAR(param.x) << VAR(param.y);

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return;
    }

    ThriftController::ClickParam click_param;
    click_param.point.x = param.x;
    click_param.point.y = param.y;

    client_->click(click_param);
}

void CustomThriftController::_swipe(SwipeParam param)
{
    LogFunc << VAR(param.steps.size()) << VAR(param.steps.front()) << VAR(param.steps.back());

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return;
    }

    ThriftController::SwipeParam swipe_param;
    for (const auto& step : param.steps) {
        ThriftController::SwipeStep thrift_step;
        thrift_step.point.x = step.x;
        thrift_step.point.y = step.y;
        thrift_step.delay = step.delay;
        swipe_param.steps.push_back(thrift_step);
    }

    client_->swipe(swipe_param);
}

void CustomThriftController::_press_key(PressKeyParam param)
{
    LogFunc;

    if (!client_ || !transport_->isOpen()) {
        LogError << "client_ is nullptr or transport_ is not open";
        return;
    }

    ThriftController::PressKeyParam thrift_param;
    thrift_param.keycode = param.keycode;

    client_->press_key(thrift_param);
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

    cv::Mat orig_mat(img.size.width, img.size.height, img.type, img.data.data());
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

#endif // WITH_THRIFT
