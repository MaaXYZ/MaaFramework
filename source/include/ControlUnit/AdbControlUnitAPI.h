#pragma once

#include <memory>
#include <optional>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_ADB_CTRL_UNIT_NS_BEGIN

/* General */

class ConnectionAPI
{
public:
    virtual ~ConnectionAPI() = default;

    virtual bool connect() = 0;
    virtual bool kill_server() = 0;
};

struct DeviceResolution
{
    int width = 0;
    int height = 0;
};

class DeviceInfoAPI
{
public:
    virtual ~DeviceInfoAPI() = default;

    virtual std::optional<std::string> request_uuid() = 0;
    virtual std::optional<DeviceResolution> request_resolution() = 0;
    virtual std::optional<int> request_orientation() = 0;

    virtual std::string get_uuid() const = 0;
    virtual DeviceResolution get_resolution() const = 0;
    virtual int get_orientation() const = 0;
};

class ActivityAPI
{
public:
    virtual ~ActivityAPI() = default;

    virtual bool start(const std::string& intent) = 0;
    virtual bool stop(const std::string& intent) = 0;
};

/* Input */

class TouchInputAPI
{
public:
    virtual ~TouchInputAPI() = default;

    virtual bool init(int swidth, int sheight, int orientation) = 0;
    virtual void deinit() = 0;
    virtual bool set_wh(int swidth, int sheight, int orientation) = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;
};

class KeyInputAPI
{
public:
    virtual ~KeyInputAPI() = default;

    virtual bool press_key(int key) = 0;
};

/* Screencap */

class ScreencapAPI
{
public:
    virtual ~ScreencapAPI() = default;

    virtual bool init(int swidth, int sheight) = 0;
    virtual void deinit() = 0;
    virtual bool set_wh(int swidth, int sheight) = 0;

    virtual std::optional<cv::Mat> screencap() = 0;
};

/* Main */

class DeviceListAPI
{
public:
    using Devices = std::vector<std::string>;

    virtual ~DeviceListAPI() = default;

    virtual std::optional<Devices> request_devices() = 0;

    virtual Devices get_devices() const = 0;
};

class ControlUnitAPI
{
public:
    virtual ~ControlUnitAPI() = default;

    virtual std::shared_ptr<ConnectionAPI> connection_obj() = 0;
    virtual std::shared_ptr<DeviceInfoAPI> device_info_obj() = 0;
    virtual std::shared_ptr<ActivityAPI> activity_obj() = 0;
    virtual std::shared_ptr<TouchInputAPI> touch_input_obj() = 0;
    virtual std::shared_ptr<KeyInputAPI> key_input_obj() = 0;
    virtual std::shared_ptr<ScreencapAPI> screencap_obj() = 0;
};

std::shared_ptr<DeviceListAPI> MAA_CONTROL_UNIT_API create_device_list_obj(MaaStringView adb_path,
                                                                               MaaStringView config);
std::shared_ptr<ControlUnitAPI> MAA_CONTROL_UNIT_API create_controller_unit(MaaStringView adb_path,
                                                                                MaaStringView adb_serial,
                                                                                MaaAdbControllerType type,
                                                                                MaaStringView config,
                                                                                MaaStringView agent_path);
std::shared_ptr<ConnectionAPI> MAA_CONTROL_UNIT_API create_connection(MaaStringView adb_path,
                                                                          MaaStringView adb_serial,
                                                                          MaaAdbControllerType type,
                                                                          MaaStringView config);
std::shared_ptr<DeviceInfoAPI> MAA_CONTROL_UNIT_API create_device_info(MaaStringView adb_path,
                                                                           MaaStringView adb_serial,
                                                                           MaaAdbControllerType type,
                                                                           MaaStringView config);
std::shared_ptr<ActivityAPI> MAA_CONTROL_UNIT_API create_activity(MaaStringView adb_path, MaaStringView adb_serial,
                                                                      MaaAdbControllerType type, MaaStringView config);
std::shared_ptr<TouchInputAPI> MAA_CONTROL_UNIT_API create_touch_input(MaaStringView adb_path,
                                                                           MaaStringView adb_serial,
                                                                           MaaAdbControllerType type,
                                                                           MaaStringView config,
                                                                           MaaStringView agent_path);
std::shared_ptr<KeyInputAPI> MAA_CONTROL_UNIT_API create_key_input(MaaStringView adb_path, MaaStringView adb_serial,
                                                                       MaaAdbControllerType type, MaaStringView config,
                                                                       MaaStringView agent_path);
std::shared_ptr<ScreencapAPI> MAA_CONTROL_UNIT_API create_screencap(MaaStringView adb_path,
                                                                        MaaStringView adb_serial,
                                                                        MaaAdbControllerType type, MaaStringView config,
                                                                        MaaStringView agent_path);

MAA_ADB_CTRL_UNIT_NS_END
