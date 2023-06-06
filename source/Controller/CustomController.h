#pragma once
#include "ControllerMgr.h"
#include "MaaCustomController.h"

MAA_CTRL_NS_BEGIN

class CustomController : public ControllerMgr
{
public:
    CustomController(MaaCustomControllerHandle handle, MaaControllerCallback callback, void* callback_arg);
    virtual ~CustomController() override = default;

protected:
    virtual bool _connect() override;
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;
    virtual cv::Mat _screencap() override;

private:
    MaaCustomControllerHandle handle_;
};

MAA_CTRL_NS_END
