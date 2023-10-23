#pragma once

#include <memory>
#include <optional>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_DBG_CTRL_UNIT_NS_BEGIN

/* General */

struct DeviceResolution
{
    int width = 0;
    int height = 0;
};

class DeviceInfoAPI
{
public:
    virtual ~DeviceInfoAPI() = default;

    virtual std::string get_uuid() const = 0;
    virtual DeviceResolution get_resolution() const = 0;
    virtual int get_orientation() const = 0;
};

/* Screencap */

class ScreencapAPI
{
public:
    virtual ~ScreencapAPI() = default;

    virtual bool init(int swidth, int sheight) = 0;

    virtual std::optional<cv::Mat> screencap() = 0;
};

/* Main */

class ControlUnitAPI
{
public:
    virtual ~ControlUnitAPI() = default;

    // virtual std::shared_ptr<ConnectionAPI> connection_obj() = 0;
    virtual std::shared_ptr<DeviceInfoAPI> device_info_obj() = 0;
    // virtual std::shared_ptr<ActivityAPI> activity_obj() = 0;
    //virtual std::shared_ptr<TouchInputAPI> touch_input_obj() = 0;
    //virtual std::shared_ptr<KeyInputAPI> key_input_obj() = 0;
    virtual std::shared_ptr<ScreencapAPI> screencap_obj() = 0;
};

std::shared_ptr<ControlUnitAPI> MAA_CONTROL_UNIT_API create_controller_unit(MaaStringView read_path,
                                                                            MaaStringView write_path,
                                                                            MaaDebuggingControllerType type,
                                                                            MaaStringView config);

MAA_DBG_CTRL_UNIT_NS_END
