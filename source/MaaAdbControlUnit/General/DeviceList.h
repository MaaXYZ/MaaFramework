#pragma once

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class DeviceList : public UnitBase
{
public:
    virtual ~DeviceList() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public:
    std::optional<std::vector<std::string>> request_devices();

private:
    ProcessArgvGenerator devices_argv_;
};

MAA_CTRL_UNIT_NS_END
