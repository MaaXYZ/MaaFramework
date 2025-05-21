#pragma once

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class AdbCommand : public UnitBase
{
public:
    virtual ~AdbCommand() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public:
    std::optional<std::string> shell(const std::string& cmd);

private:
    ProcessArgvGenerator shell_argv;
};

MAA_CTRL_UNIT_NS_END
