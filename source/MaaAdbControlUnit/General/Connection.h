#pragma once

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class Connection : public UnitBase
{
public:
    virtual ~Connection() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public:
    bool connect();
    bool kill_server();

private:
    ProcessArgvGenerator connect_argv_;
    ProcessArgvGenerator kill_server_argv_;
};

MAA_CTRL_UNIT_NS_END
