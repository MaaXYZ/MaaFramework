#pragma once

#include "UnitBase.h"

MAA_ADB_CTRL_UNIT_NS_BEGIN

class DeviceList : public DeviceListBase
{
public:
    virtual ~DeviceList() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from DeviceListAPI
    virtual std::optional<Devices> request_devices() override;

    virtual Devices get_devices() const override;

private:
    Argv devices_argv_;

private:
    Devices devices_;
};

MAA_ADB_CTRL_UNIT_NS_END
