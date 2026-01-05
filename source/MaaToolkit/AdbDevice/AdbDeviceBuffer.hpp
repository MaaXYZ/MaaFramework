#pragma once

#include <ostream>

#include <meojson/json.hpp>

#include "API/MaaToolkitBufferTypes.hpp"
#include "Common/Conf.h"
#include "MaaUtils/Buffer/ListBuffer.hpp"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

struct AdbDevice
{
    std::string name;
    std::filesystem::path adb_path;
    std::string serial;
    MaaAdbScreencapMethod screencap_methods = MaaAdbScreencapMethod_None;
    MaaAdbInputMethod input_methods = MaaAdbInputMethod_None;
    json::object config;

    MEO_TOJSON(name, adb_path, serial, screencap_methods, input_methods, config);
};

class AdbDeviceBuffer : public MaaToolkitAdbDevice
{
public:
    AdbDeviceBuffer(const AdbDevice& device)
        : name_(device.name)
        , adb_path_(path_to_utf8_string(device.adb_path))
        , address_(device.serial)
        , screencap_methods_(device.screencap_methods)
        , input_methods_(device.input_methods)
        , config_(device.config.to_string())
    {
    }

    virtual ~AdbDeviceBuffer() override = default;

    virtual const std::string& name() const override { return name_; }

    virtual const std::string& adb_path() const override { return adb_path_; }

    virtual const std::string& address() const override { return address_; }

    virtual MaaAdbScreencapMethod screencap_methods() const override { return screencap_methods_; }

    virtual MaaAdbInputMethod input_methods() const override { return input_methods_; }

    virtual const std::string& config() const override { return config_; }

private:
    std::string name_;
    std::string adb_path_;
    std::string address_;
    MaaAdbScreencapMethod screencap_methods_ = MaaAdbScreencapMethod_None;
    MaaAdbInputMethod input_methods_ = MaaAdbInputMethod_None;
    std::string config_;
};

MAA_TOOLKIT_NS_END

struct MaaToolkitAdbDeviceList : public MAA_NS::ListBuffer<MAA_TOOLKIT_NS::AdbDeviceBuffer>
{
    virtual ~MaaToolkitAdbDeviceList() override = default;
};
