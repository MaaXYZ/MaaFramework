#pragma once
#include "ControllerMgr.h"

#include "ControlUnit/AdbControlUnitAPI.h"

#include <memory>

MAA_CTRL_NS_BEGIN

class AdbController : public ControllerMgr
{
public:
    AdbController(std::string adb_path, std::string address,
                  std::shared_ptr<MAA_ADB_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr, MaaControllerCallback callback,
                  MaaCallbackTransparentArg callback_arg);
    virtual ~AdbController() override;

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
    bool reinit_resolution();

    std::string adb_path_;
    std::string address_;
    std::pair<int, int> resolution_ = { 0, 0 };
    std::shared_ptr<MAA_ADB_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr_ = nullptr;
};

MAA_CTRL_NS_END
