#pragma once

#include "ControlUnit/DebuggingControlUnitAPI.h"

MAA_DBG_CTRL_UNIT_NS_BEGIN

class ControlUnitMgr : public ControlUnitAPI
{
public:
    virtual ~ControlUnitMgr() override = default;

public: // from ControlUnitAPI
    virtual std::shared_ptr<DeviceInfoAPI> device_info_obj() override { return device_info_; }
    virtual std::shared_ptr<ScreencapAPI> screencap_obj() override { return screencap_; }

public:
    void set_device_info_obj(std::shared_ptr<DeviceInfoAPI> obj) { device_info_ = std::move(obj); }
    void set_screencap_obj(std::shared_ptr<ScreencapAPI> obj) { screencap_ = std::move(obj); }

private:
    std::shared_ptr<DeviceInfoAPI> device_info_ = nullptr;
    std::shared_ptr<ScreencapAPI> screencap_ = nullptr;
};

MAA_DBG_CTRL_UNIT_NS_END
