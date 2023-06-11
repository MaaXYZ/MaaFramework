#pragma once
#include "ControllerMgr.h"

MAA_CTRL_NS_BEGIN

class MinitouchController : public ControllerMgr
{
public:
    MinitouchController(const std::string& adb_path, const std::string& address, const json::value& config,
                        MaaControllerCallback callback, void* callback_arg);
    virtual ~MinitouchController() override;

protected:
    virtual bool _connect() override;
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;
    virtual cv::Mat _screencap() override;
};

MAA_CTRL_NS_END
