#pragma once

#include "../UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class Activity : public UnitBase
{
public:
    bool parse(const json::value& config);

    bool start(const std::string& intent);
    bool stop(const std::string& intent);

private:
    Argv start_app_argv_;
    Argv stop_app_argv_;
};

MAA_CTRL_UNIT_NS_END