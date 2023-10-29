#pragma once

#ifdef WITH_THRIFT_CONTROLLER

#include "ControllerMgr.h"

#include "ThriftController.h"

MAA_CTRL_NS_BEGIN

class CustomThriftController : public ControllerMgr
{
public:
    CustomThriftController(const std::string& param, MaaControllerCallback callback, void* callback_arg);
    virtual ~CustomThriftController() override;

protected:
    virtual bool _connect() override;
    virtual std::string _get_uuid() const override;
    virtual std::pair<int, int> _get_resolution() const override;
    virtual bool _click(ClickParam param) override;
    virtual bool _swipe(SwipeParam param) override;
    virtual bool _touch_down(TouchParam param) override;
    virtual bool _touch_move(TouchParam param) override;
    virtual bool _touch_up(TouchParam param) override;
    virtual bool _press_key(PressKeyParam param) override;
    virtual cv::Mat _screencap() override;
    virtual bool _start_app(AppParam param) override;
    virtual bool _stop_app(AppParam param) override;

private:
    std::shared_ptr<ThriftController::ThriftControllerClient> client_ = nullptr;
    std::shared_ptr<apache::thrift::transport::TTransport> transport_ = nullptr;

    enum ThriftControllerTypeEnum
    {
        // param format should be "host:port"
        MaaThriftControllerType_Socket = 1,

        // param should be unix domain socket path
        MaaThriftControllerType_UnixDomainSocket = 2,
    };
};

MAA_CTRL_NS_END

#endif // WITH_THRIFT_CONTROLLER
