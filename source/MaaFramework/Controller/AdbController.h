#pragma once
#include "ControllerMgr.h"

#include "MaaControlUnit/ControlUnitAPI.h"

#include <memory>

MAA_CTRL_NS_BEGIN

class AdbController : public ControllerMgr
{
public:
    AdbController(std::string adb_path, std::string address, std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr,
                  MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg);
    virtual ~AdbController() override;

    virtual std::string get_uuid() const override;

protected:
    virtual bool _connect() override;
    virtual std::pair<int, int> _get_resolution() const override;
    virtual void _click(ClickParam param) override;
    virtual void _swipe(SwipeParam param) override;
    virtual void _press_key(PressKeyParam param) override;
    virtual cv::Mat _screencap() override;
    virtual bool _start_app(AppParam param) override;
    virtual bool _stop_app(AppParam param) override;

private:
    std::string adb_path_;
    std::string address_;
    std::pair<int, int> resolution_ = { 0, 0 };
    std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr_ = nullptr;
};

MAA_CTRL_NS_END
