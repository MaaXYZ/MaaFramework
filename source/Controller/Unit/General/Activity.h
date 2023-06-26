#pragma once

#include "../UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MAAAPI Activity : public UnitBase
{
public:
    virtual ~Activity() override = default;

    virtual bool parse(const json::value& config) override;

    bool start(const std::string& intent);
    bool stop(const std::string& intent);

private:
    Argv start_app_argv_;
    Argv stop_app_argv_;
};

MAA_CTRL_UNIT_NS_END
