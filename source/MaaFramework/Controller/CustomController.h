#pragma once
#include "ControllerMgr.h"
#include "MaaFramework/MaaCustomController.h"

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
    MaaCustomControllerHandle handle_ = nullptr;
};

MAA_CTRL_NS_END
