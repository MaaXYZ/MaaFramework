#pragma once

#include "UnitBase.h"

#include "Invoke/InvokeApp.h"

MAA_CTRL_UNIT_NS_BEGIN

class MaatouchInput : public TouchInputBase, public KeyInputBase
{
public:
    MaatouchInput()
    {
        TouchInputBase::children_.emplace_back(invoke_app_);
        KeyInputBase::children_.emplace_back(invoke_app_);
    }
    virtual ~MaatouchInput() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

    virtual void set_io(std::shared_ptr<PlatformIO> io_ptr) override
    {
        TouchInputBase::set_io(io_ptr);
        KeyInputBase::set_io(io_ptr);
    }
    virtual void set_replacement(Argv::replacement argv_replace) override
    {
        TouchInputBase::set_replacement(argv_replace);
        KeyInputBase::set_replacement(argv_replace);
    }
    virtual void merge_replacement(Argv::replacement argv_replace, bool _override = true) override
    {
        TouchInputBase::merge_replacement(argv_replace, _override);
        KeyInputBase::merge_replacement(argv_replace, _override);
    }

public: // from TouchInputAPI
    virtual bool init(int swidth, int sheight, int orientation) override;
    virtual void deinit() override {}
    virtual void set_wh(int swidth, int sheight, int orientation) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(const std::vector<SwipeStep>& steps) override;

public: // from KeyInputAPI
    virtual bool press_key(int key) override;

private:
    std::pair<int, int> scale_point(int x, int y);

    std::shared_ptr<InvokeApp> invoke_app_ = std::make_shared<InvokeApp>();
    std::shared_ptr<IOHandler> shell_handler_ = nullptr;

    std::string root_;
    std::string package_name_;
    int swidth_ = 0;  // screen width
    int sheight_ = 0; // screen height
    double xscale_ = 0;
    double yscale_ = 0;
    int press_ = 0;
};

MAA_CTRL_UNIT_NS_END
