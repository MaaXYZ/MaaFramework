#pragma once

#include "../UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MAAAPI_DEBUG Connection : public UnitBase
{
public:
    virtual ~Connection() override = default;

    virtual bool parse(const json::value& config) override;

    bool connect();
    bool kill_server();

private:
    Argv connect_argv_;
    Argv kill_server_argv_;
};

MAA_CTRL_UNIT_NS_END
