#pragma once

#include "ControlUnit/DebuggingControlUnitAPI.h"

#include <filesystem>

MAA_DBG_CTRL_UNIT_NS_BEGIN

class VirtualInfo : public DeviceInfoAPI
{
public:
    VirtualInfo(std::filesystem::path path) : path_(std::move(path)) {}
    virtual ~VirtualInfo() override = default;

public: // from DeviceInfoAPI
    virtual std::string get_uuid() const override { return uuid_; }
    virtual DeviceResolution get_resolution() const override { return resolution_; }
    virtual int get_orientation() const override { return orientation_; }

public:
    bool parse(const json::value& config);

private:
    std::filesystem::path path_;

    std::string uuid_;
    DeviceResolution resolution_;
    int orientation_;
};

MAA_DBG_CTRL_UNIT_NS_END
