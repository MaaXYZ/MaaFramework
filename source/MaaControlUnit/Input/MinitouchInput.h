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
    virtual bool swipe(const std::vector<SwipeStep>& steps) override;

private:
    std::pair<int, int> scale_point(int x, int y);

    std::shared_ptr<InvokeApp> invoke_app_ = std::make_shared<InvokeApp>();
    std::shared_ptr<IOHandler> shell_handler_ = nullptr;

    std::string root_;
    std::vector<std::string> arch_list_;
    int swidth_ = 0;  // screen width
    int sheight_ = 0; // screen height
    int twidth_ = 0;  // touch width
    int theight_ = 0; // touch height
    double xscale_ = 0;
    double yscale_ = 0;
    int press_ = 0;
    int orientation_ = 0;
};

MAA_CTRL_UNIT_NS_END
