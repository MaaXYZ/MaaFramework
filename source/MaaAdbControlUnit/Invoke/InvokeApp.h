#pragma once

#include "Base/UnitBase.h"
#include "Utils/IOStream/ChildPipeIOStream.h"

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

    std::optional<std::string> invoke_bin_and_read_pipe(const std::string& extra);
    std::shared_ptr<ChildPipeIOStream> invoke_bin(const std::string& extra);
    std::shared_ptr<ChildPipeIOStream> invoke_app(const std::string& package);

private:
    ProcessArgvGenerator abilist_argv_;
    ProcessArgvGenerator sdk_argv_;
    ProcessArgvGenerator push_bin_argv_;
    ProcessArgvGenerator chmod_bin_argv_;
    ProcessArgvGenerator invoke_bin_argv_;
    ProcessArgvGenerator invoke_app_argv_;

    std::string tempname_;
};

MAA_CTRL_UNIT_NS_END
