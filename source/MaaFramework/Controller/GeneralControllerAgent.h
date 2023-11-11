#pragma once
#include "ControllerAgent.h"

#include "ControlUnit/DbgControlUnitAPI.h"
#include "Utils/LibraryHolder.h"

#include <memory>

MAA_CTRL_NS_BEGIN

class GeneralControllerAgent : public ControllerAgent
{
public:
    GeneralControllerAgent(std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> control_unit,
                           MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg);
    virtual ~GeneralControllerAgent() override = default;

protected:
    virtual bool _connect() override;
    virtual std::optional<std::string> _request_uuid() override;
    virtual std::optional<std::pair<int, int>> _request_resolution() override;
    virtual bool _start_app(AppParam param) override;
    virtual bool _stop_app(AppParam param) override;
    virtual std::optional<cv::Mat> _screencap() override;
    virtual bool _click(ClickParam param) override;
    virtual bool _swipe(SwipeParam param) override;
    virtual bool _touch_down(TouchParam param) override;
    virtual bool _touch_move(TouchParam param) override;
    virtual bool _touch_up(TouchParam param) override;
    virtual bool _press_key(PressKeyParam param) override;

private:
    std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> control_unit_ = nullptr;
};

MAA_CTRL_NS_END
