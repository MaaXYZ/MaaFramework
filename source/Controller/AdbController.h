#pragma once
#include "ControllerMgr.h"

#include "ControlUnitAPI.h"

#include <memory>

MAA_CTRL_NS_BEGIN

class AdbController : public ControllerMgr
{
public:
    AdbController(std::string adb_path, std::string address, std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr,
                  MaaControllerCallback callback, void* callback_arg);
    virtual ~AdbController() override;

    virtual std::string get_uuid() const override;

protected:
    virtual bool _connect() override;
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;
    virtual cv::Mat _screencap() override;

private:
    std::string adb_path_;
    std::string address_;
    std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr_ = nullptr;
};

MAA_CTRL_NS_END
