#pragma once

#include <filesystem>

#include "Base/UnitBase.h"
#include "Invoke/InvokeApp.h"
#include "MtouchHelper.h"

MAA_CTRL_UNIT_NS_BEGIN

class MaatouchInput
    : public MtouchHelper
    , public KeyInputBase
{
public:
    explicit MaatouchInput(std::filesystem::path agent_path)
        : agent_path_(std::move(agent_path))
    {
        TouchInputBase::children_.emplace_back(invoke_app_);
        KeyInputBase::children_.emplace_back(invoke_app_);
    }

    virtual ~MaatouchInput() override;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

    virtual void set_replacement(UnitBase::Replacement argv_replace) override
    {
        TouchInputBase::set_replacement(argv_replace);
        KeyInputBase::set_replacement(argv_replace);
    }

    virtual void
        merge_replacement(UnitBase::Replacement argv_replace, bool _override = true) override
    {
        TouchInputBase::merge_replacement(argv_replace, _override);
        KeyInputBase::merge_replacement(argv_replace, _override);
    }

public: // from TouchInputAPI
    virtual bool init() override;
    virtual void deinit() override;

public: // from KeyInputAPI
    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

protected: // from MtouchHelper
    virtual std::pair<int, int> screen_to_touch(int x, int y) override
    {
        return _screen_to_touch(x, y);
    }

    virtual std::pair<int, int> screen_to_touch(double x, double y) override
    {
        return _screen_to_touch(x, y);
    }

private:
    template <typename T1, typename T2>
    inline std::pair<int, int> _screen_to_touch(T1 x, T2 y)
    {
        return std::make_pair(
            static_cast<int>(round(x * xscale_)),
            static_cast<int>(round(y * yscale_)));
    }

    void remove_binary();

    std::filesystem::path agent_path_;
    std::string package_name_;
    std::shared_ptr<InvokeApp> invoke_app_ = std::make_shared<InvokeApp>();
};

MAA_CTRL_UNIT_NS_END
