#pragma once

#include "ControlUnit/AdbControlUnitAPI.h"

#include "UnitBase.h"

MAA_ADB_CTRL_UNIT_NS_BEGIN

class ControlUnitMgr : public ControlUnitAPI
{
public:
    virtual ~ControlUnitMgr() override = default;

public: // from ControlUnitAPI
    virtual std::shared_ptr<ConnectionAPI> connection_obj() override { return connection_; }
    virtual std::shared_ptr<DeviceInfoAPI> device_info_obj() override { return device_info_; }
    virtual std::shared_ptr<ActivityAPI> activity_obj() override { return activity_; }
    virtual std::shared_ptr<TouchInputAPI> touch_input_obj() override { return touch_input_; }
    virtual std::shared_ptr<KeyInputAPI> key_input_obj() override { return key_input_; }
    virtual std::shared_ptr<ScreencapAPI> screencap_obj() override { return screencap_; }

public:
    bool parse(const json::value& config);
    void set_io(const std::shared_ptr<PlatformIO>& io_ptr);
    void set_replacement(const std::map<std::string, std::string>& replacement);

    void set_connection_obj(std::shared_ptr<ConnectionBase> obj) { connection_ = std::move(obj); }
    void set_device_info_obj(std::shared_ptr<DeviceInfoBase> obj) { device_info_ = std::move(obj); }
    void set_activity_obj(std::shared_ptr<ActivityBase> obj) { activity_ = std::move(obj); }
    void set_touch_input_obj(std::shared_ptr<TouchInputBase> obj) { touch_input_ = std::move(obj); }
    void set_key_input_obj(std::shared_ptr<KeyInputBase> obj) { key_input_ = std::move(obj); }
    void set_screencap_obj(std::shared_ptr<ScreencapBase> obj) { screencap_ = std::move(obj); }

private:
    std::shared_ptr<ConnectionBase> connection_ = nullptr;
    std::shared_ptr<DeviceInfoBase> device_info_ = nullptr;
    std::shared_ptr<ActivityBase> activity_ = nullptr;
    std::shared_ptr<TouchInputBase> touch_input_ = nullptr;
    std::shared_ptr<KeyInputBase> key_input_ = nullptr;
    std::shared_ptr<ScreencapBase> screencap_ = nullptr;
};

MAA_ADB_CTRL_UNIT_NS_END
