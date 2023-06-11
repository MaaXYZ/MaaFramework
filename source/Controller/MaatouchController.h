#pragma once
#include "ControllerMgr.h"

MAA_CTRL_NS_BEGIN

class MaatouchController : public ControllerMgr
{
public:
    MaatouchController(const std::string& adb_path, const std::string& address, const json::value& config,
                       MaaControllerCallback callback, void* callback_arg);
    virtual ~MaatouchController() override;

protected:
    virtual bool _connect() override;
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;
    virtual cv::Mat _screencap() override;
};

MAA_CTRL_NS_END
