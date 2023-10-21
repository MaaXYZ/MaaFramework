#pragma once

#include "UnitBase.h"

MAA_ADB_CTRL_UNIT_NS_BEGIN

class DeviceInfo : public DeviceInfoBase
{
public:
    virtual ~DeviceInfo() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from DeviceInfoAPI
    virtual std::optional<std::string> request_uuid() override;
    virtual std::optional<DeviceResolution> request_resolution() override;
    virtual std::optional<int> request_orientation() override;

    virtual std::string get_uuid() const override { return uuid_; }
    virtual DeviceResolution get_resolution() const override { return resolution_; }
    virtual int get_orientation() const override { return orientation_; }

private:
    Argv uuid_argv_;
    Argv resolution_argv_;
    Argv orientation_argv_;

private:
    std::string uuid_;
    DeviceResolution resolution_;
    int orientation_ = -1;
};

MAA_ADB_CTRL_UNIT_NS_END
