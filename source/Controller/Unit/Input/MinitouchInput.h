#pragma once

#include "../UnitBase.h"

#include "../Utils/InvokeApp.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinitouchInput : public UnitBase
{
public:
    struct Step
    {
        int x, y;
        int delay;
    };

    MinitouchInput() : invoke_app_(new InvokeApp) { children_.push_back(invoke_app_.get()); }

    bool parse(const json::value& config);

    bool init(int swidth, int sheight, std::function<std::string(const std::string&)> path_of_bin,
              const std::string& force_temp = "");

    bool click(int x, int y);
    bool swipe(const std::vector<Step>& steps);
    bool press_key(int key);

private:
    void scalePoint(int& x, int& y);

    std::shared_ptr<InvokeApp> invoke_app_;
    std::shared_ptr<IOHandler> shell_handler_;
    int width, height;
    double xscale, yscale, press;
};

MAA_CTRL_UNIT_NS_END
