#pragma once

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class Activity : public UnitBase
{
public:
    virtual ~Activity() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public:
    bool start_app(const std::string& intent);
    bool stop_app(const std::string& intent);

private:
    ProcessArgvGenerator start_app_argv_;
    ProcessArgvGenerator start_activity_argv_;
    ProcessArgvGenerator stop_app_argv_;
};

MAA_CTRL_UNIT_NS_END
