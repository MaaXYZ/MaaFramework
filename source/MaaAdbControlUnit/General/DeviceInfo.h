#pragma once

#include "Base/UnitBase.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class DeviceInfo : public UnitBase
{
public:
    struct DisplayInfo
    {
        int w = 0;
        int h = 0;
        int r = 0;
    };

public:
    virtual ~DeviceInfo() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public:
    std::optional<std::string> request_uuid();
    std::optional<DisplayInfo> request_resolution();
    std::optional<int> request_orientation();

private:
    ProcessArgvGenerator uuid_argv_;
    ProcessArgvGenerator resolution_argv_;
    ProcessArgvGenerator orientation_argv_;
};

MAA_CTRL_UNIT_NS_END
