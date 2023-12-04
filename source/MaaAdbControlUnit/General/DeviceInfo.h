#pragma once

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class DeviceInfo : public UnitBase
{
public:
    virtual ~DeviceInfo() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public:
    std::optional<std::string> request_uuid();
    std::optional<std::pair<int, int>> request_resolution();
    std::optional<int> request_orientation();

private:
    ProcessArgvGenerator uuid_argv_;
    ProcessArgvGenerator resolution_argv_;
    ProcessArgvGenerator orientation_argv_;
};

MAA_CTRL_UNIT_NS_END
