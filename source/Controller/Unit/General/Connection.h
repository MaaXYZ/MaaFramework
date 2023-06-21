#pragma once

#include "../UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class Connection : public UnitBase
{
public:
    bool parse(const json::value& config) override;

    bool connect();
    bool kill_server();

private:
    Argv connect_argv_;
    Argv kill_server_argv_;
};

MAA_CTRL_UNIT_NS_END
