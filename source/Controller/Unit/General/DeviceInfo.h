#pragma once

#include "../UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class DeviceInfo : public UnitBase
{
public:
    struct Resolution
    {
        int width = 0;
        int height = 0;
    };

    bool parse(const json::value& config) override;

    std::optional<std::string> request_uuid();
    std::optional<Resolution> request_resolution();
    std::optional<int> request_orientation();

public:
    const std::string& get_uuid() const { return uuid_; }
    const Resolution& get_resolution() const { return resolution_; }
    int get_orientation() const { return orientation_; }

private:
    Argv uuid_argv_;
    Argv resolution_argv_;
    Argv orientation_argv_;

private:
    std::string uuid_;
    Resolution resolution_;
    int orientation_;
};

MAA_CTRL_UNIT_NS_END
