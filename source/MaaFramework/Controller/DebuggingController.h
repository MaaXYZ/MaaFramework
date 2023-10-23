#pragma once
#include "ControllerMgr.h"

#include "ControlUnit/DebuggingControlUnitAPI.h"

#include <memory>

MAA_CTRL_NS_BEGIN

class DebuggingController : public ControllerMgr
{
public:
    DebuggingController(std::string read_path, std::string write_path,
                        std::shared_ptr<MAA_DBG_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr, MaaControllerCallback callback,
                        MaaCallbackTransparentArg callback_arg);
    virtual ~DebuggingController() override;

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
    std::string read_path_;
    std::string write_path_;

    std::shared_ptr<MAA_DBG_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr_ = nullptr;
};

MAA_CTRL_NS_END
