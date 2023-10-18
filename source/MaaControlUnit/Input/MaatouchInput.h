#pragma once

#include "UnitBase.h"

#include <filesystem>

#include "Invoke/InvokeApp.h"

MAA_CTRL_UNIT_NS_BEGIN

class MaatouchInput : public TouchInputBase, public KeyInputBase
{
public:
    MaatouchInput(std::filesystem::path agent_path) : agent_path_(std::move(agent_path))
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
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

public: // from KeyInputAPI
    virtual bool press_key(int key) override;

private:
    template <typename T1, typename T2>
    inline std::pair<int, int> screen_to_touch(T1 x, T2 y)
    {
        return std::make_pair(static_cast<int>(round(x * xscale_)), static_cast<int>(round(y * yscale_)));
    }

    std::shared_ptr<InvokeApp> invoke_app_ = std::make_shared<InvokeApp>();
    std::shared_ptr<IOHandler> shell_handler_ = nullptr;

    std::filesystem::path agent_path_;
    std::string package_name_;
    int screen_width_ = 0;
    int screen_height_ = 0;
    int touch_width_ = 0;
    int touch_height_ = 0;
    double xscale_ = 0;
    double yscale_ = 0;
    int press_ = 0;
    int orientation_ = 0;
};

MAA_CTRL_UNIT_NS_END
