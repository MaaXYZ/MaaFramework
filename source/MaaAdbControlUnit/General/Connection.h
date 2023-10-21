#pragma once

#include "UnitBase.h"

MAA_ADB_CTRL_UNIT_NS_BEGIN

class Connection : public ConnectionBase
{
public:
    virtual ~Connection() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from ConnectionAPI
    virtual bool connect() override;
    virtual bool kill_server() override;

private:
    Argv connect_argv_;
    Argv kill_server_argv_;
};

MAA_ADB_CTRL_UNIT_NS_END
