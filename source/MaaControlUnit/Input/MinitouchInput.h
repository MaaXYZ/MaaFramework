#pragma once

#include "UnitBase.h"

#include "Invoke/InvokeApp.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinitouchInput : public TouchInputBase
{
public:
    MinitouchInput() { children_.emplace_back(invoke_app_); }
    virtual ~MinitouchInput() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from TouchInputAPI
    virtual bool init(int swidth, int sheight, int orientation) override;
    virtual void deinit() override {}
    virtual void set_wh(int swidth, int sheight, int orientation) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

private:
    template <typename T1, typename T2>
    inline std::pair<int, int> screen_to_touch(T1 x, T2 y)
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

    std::shared_ptr<InvokeApp> invoke_app_ = std::make_shared<InvokeApp>();
    std::shared_ptr<IOHandler> shell_handler_ = nullptr;

    std::string root_;
    std::vector<std::string> arch_list_;
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
