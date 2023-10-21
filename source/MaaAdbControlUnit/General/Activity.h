#pragma once

#include "UnitBase.h"

MAA_ADB_CTRL_UNIT_NS_BEGIN

class Activity : public ActivityBase
{
public:
    virtual ~Activity() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ActivityAPI
    virtual bool start(const std::string& intent) override;
    virtual bool stop(const std::string& intent) override;

private:
    Argv start_app_argv_;
    Argv stop_app_argv_;
};

MAA_ADB_CTRL_UNIT_NS_END
