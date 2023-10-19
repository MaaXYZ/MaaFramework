#pragma once

#include "MtouchHelper.h"

#include <filesystem>

#include "Invoke/InvokeApp.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinitouchInput : public MtouchHelper
{
public:
    MinitouchInput(std::filesystem::path agent_path) : agent_path_(std::move(agent_path))
    {
        children_.emplace_back(invoke_app_);
    }
    virtual ~MinitouchInput() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from TouchInputAPI
    virtual bool init(int swidth, int sheight, int orientation) override;
    virtual void deinit() override {}
    virtual bool set_wh(int swidth, int sheight, int orientation) override;

protected: // from MtouchHelper
    virtual std::pair<int, int> screen_to_touch(int x, int y) override { return _screen_to_touch(x, y); }
    virtual std::pair<int, int> screen_to_touch(double x, double y) override { return _screen_to_touch(x, y); }

private:
    template <typename T1, typename T2>
    inline std::pair<int, int> _screen_to_touch(T1 x, T2 y)
    {
        auto make_pair = [](double x, double y) {
            return std::make_pair(static_cast<int>(round(x)), static_cast<int>(round(y)));
        };
        switch (orientation_) {
        case 0:
            return make_pair(x * xscale_, y * yscale_);
        case 1:
            return make_pair(touch_height_ - y * yscale_, x * xscale_);
        case 2:
            return make_pair(touch_width_ - x * xscale_, touch_height_ - y * yscale_);
        case 3:
            return make_pair(y * yscale_, touch_width_ - x * xscale_);
        default:
            return make_pair(x * xscale_, y * yscale_);
        }
    }

    std::filesystem::path agent_path_;
    std::vector<std::string> arch_list_;
    std::shared_ptr<InvokeApp> invoke_app_ = std::make_shared<InvokeApp>();
};

MAA_CTRL_UNIT_NS_END
