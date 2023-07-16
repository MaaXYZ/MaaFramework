#pragma once
#include "ControllerMgr.h"
#include "MaaCustomController.h"

MAA_CTRL_NS_BEGIN

class CustomController : public ControllerMgr
{
public:
    CustomController(MaaCustomControllerHandle handle, MaaControllerCallback callback,
                     MaaCallbackTransparentArg callback_arg);
    virtual ~CustomController() override = default;

    virtual std::string get_uuid() const override;

protected:
    virtual bool _connect() override;
    virtual std::pair<int, int> _get_resolution() const override;
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;
    virtual void _press_key(PressKeyParams param) override;
    virtual cv::Mat _screencap() override;
    virtual bool _start_app(AppParams param) override;
    virtual bool _stop_app(AppParams param) override;

private:
    MaaCustomControllerHandle handle_ = nullptr;
};

MAA_CTRL_NS_END
