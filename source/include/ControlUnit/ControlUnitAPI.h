#pragma once

#include <memory>
#include <optional>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_CTRL_UNIT_NS_BEGIN

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

struct SwipeStep
{
    int x = 0;
    int y = 0;
    int delay = 0;
};

class TouchInputAPI
{
public:
    virtual ~TouchInputAPI() = default;

    virtual bool init(int swidth, int sheight) = 0;
    virtual void deinit() = 0;
    virtual void set_wh(int swidth, int sheight) = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(const std::vector<SwipeStep>& steps) = 0;
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
    virtual void set_wh(int swidth, int sheight) = 0;

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

std::shared_ptr<DeviceListAPI> MAA_CONTROL_UNIT_API create_adb_device_list_obj(MaaString adb_path,
                                                                               MaaString config);
std::shared_ptr<ControlUnitAPI> MAA_CONTROL_UNIT_API create_adb_controller_unit(MaaString adb_path,
                                                                                MaaString adb_serial,
                                                                                MaaAdbControllerType type,
                                                                                MaaString config);
std::shared_ptr<ConnectionAPI> MAA_CONTROL_UNIT_API create_adb_connection(MaaString adb_path, MaaString adb_serial,
                                                                          MaaAdbControllerType type,
                                                                          MaaString config);
std::shared_ptr<DeviceInfoAPI> MAA_CONTROL_UNIT_API create_adb_device_info(MaaString adb_path, MaaString adb_serial,
                                                                           MaaAdbControllerType type,
                                                                           MaaString config);
std::shared_ptr<ActivityAPI> MAA_CONTROL_UNIT_API create_adb_activity(MaaString adb_path, MaaString adb_serial,
                                                                      MaaAdbControllerType type, MaaString config);
std::shared_ptr<TouchInputAPI> MAA_CONTROL_UNIT_API create_adb_touch_input(MaaString adb_path, MaaString adb_serial,
                                                                           MaaAdbControllerType type,
                                                                           MaaString config);
std::shared_ptr<KeyInputAPI> MAA_CONTROL_UNIT_API create_adb_key_input(MaaString adb_path, MaaString adb_serial,
                                                                       MaaAdbControllerType type, MaaString config);
std::shared_ptr<ScreencapAPI> MAA_CONTROL_UNIT_API create_adb_screencap(MaaString adb_path, MaaString adb_serial,
                                                                        MaaAdbControllerType type,
                                                                        MaaString config);

MAA_CTRL_UNIT_NS_END
