#pragma once

#include "MtouchHelper.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinitouchInput : public MtouchHelper
{
public:
    explicit MinitouchInput(std::filesystem::path agent_path)
    {
        agent_path_ = std::move(agent_path);
        invoke_app_ = std::make_shared<InvokeApp>();
        adb_shell_input_ = std::make_shared<AdbShellInput>();
        children_.emplace_back(invoke_app_);
        children_.emplace_back(adb_shell_input_);
    }

    virtual ~MinitouchInput() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from InputBase
    virtual bool init() override;

public: // from ControlUnitSink
    virtual void on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur) override;

private:
    bool invoke_and_read_info();
};

MAA_CTRL_UNIT_NS_END
