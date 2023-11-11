#pragma once

#include "Base/UnitBase.h"

#include <filesystem>

MAA_CTRL_UNIT_NS_BEGIN

class InvokeApp : public UnitBase
{
public:
    virtual ~InvokeApp() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public:
    bool init(const std::string& force_temp = "");

    std::optional<std::vector<std::string>> abilist();
    std::optional<int> sdk();
    bool push(const std::filesystem::path& path);
    bool chmod();

    std::optional<std::string> invoke_bin_stdout(const std::string& extra);
    std::shared_ptr<IOHandler> invoke_bin(const std::string& extra, bool wants_stderr = false);
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
