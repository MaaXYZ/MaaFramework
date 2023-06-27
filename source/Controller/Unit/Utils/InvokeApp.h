#pragma once

#include "../UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MAA_DEBUG_API InvokeApp : public UnitBase
{
public:
    virtual ~InvokeApp() override = default;

    virtual bool parse(const json::value& config) override;

    bool init(const std::string& force_temp = "");

    std::optional<std::vector<std::string>> abilist();
    std::optional<int> sdk();
    bool push(const std::string& path);
    bool chmod();

    std::optional<std::string> invoke_bin_stdout(const std::string& extra);
    std::shared_ptr<IOHandler> invoke_bin(const std::string& extra);
    std::shared_ptr<IOHandler> invoke_app(const std::string& package);

#ifdef MAA_DEBUG
    std::string get_tempname() const { return tempname_; }
#endif

private:
    Argv abilist_argv_;
    Argv sdk_argv_;
    Argv push_bin_argv_;
    Argv chmod_bin_argv_;
    Argv invoke_bin_argv_;
    Argv invoke_app_argv_;

    std::string tempname_;
};

MAA_CTRL_UNIT_NS_END
