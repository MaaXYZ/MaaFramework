#pragma once

#include <memory>
#include <optional>

#include <meojson/json.hpp>

#include "Common/MaaConf.h"
#include "MaaDef.h"
#include "Utils/NoWarningCVMat.h"

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

    virtual bool init(int w, int h) = 0;
    virtual void deinit() = 0;
    virtual std::optional<cv::Mat> screencap() = 0;
};

/* Main */

class ControlUnitAPI
{
public:
    virtual ~ControlUnitAPI() = default;

    virtual void set_adb(const std::string& adb_path, const std::string& adb_serial) = 0;

    virtual std::shared_ptr<ConnectionAPI> connection_obj() = 0;
    virtual std::shared_ptr<DeviceInfoAPI> device_info_obj() = 0;
    virtual std::shared_ptr<ActivityAPI> activity_obj() = 0;
    virtual std::shared_ptr<TouchInputAPI> touch_input_obj() = 0;
    virtual std::shared_ptr<KeyInputAPI> key_input_obj() = 0;
    virtual std::shared_ptr<ScreencapAPI> screencap_obj() = 0;
};

std::shared_ptr<ControlUnitAPI> MAA_API create_controller_unit(MaaAdbControllerType type, MaaJsonString config);

// for debug
std::shared_ptr<ConnectionAPI> MAA_DEBUG_API create_connection(MaaAdbControllerType type, MaaJsonString config);
std::shared_ptr<DeviceInfoAPI> MAA_DEBUG_API create_device_info(MaaAdbControllerType type, MaaJsonString config);
std::shared_ptr<ActivityAPI> MAA_DEBUG_API create_activity(MaaAdbControllerType type, MaaJsonString config);
std::shared_ptr<TouchInputAPI> MAA_DEBUG_API create_touch_input(MaaAdbControllerType type, MaaJsonString config);
std::shared_ptr<KeyInputAPI> MAA_DEBUG_API create_key_input(MaaAdbControllerType type, MaaJsonString config);
std::shared_ptr<ScreencapAPI> MAA_DEBUG_API create_screencap(MaaAdbControllerType type, MaaJsonString config);

MAA_CTRL_UNIT_NS_END
