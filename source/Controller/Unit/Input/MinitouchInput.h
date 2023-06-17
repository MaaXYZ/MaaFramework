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

    MinitouchInput() { children_.emplace_back(invoke_app_); }

    bool parse(const json::value& config);

    bool init(int swidth, int sheight, std::function<std::string(const std::string&)> path_of_bin,
              const std::string& force_temp = "");

    bool click(int x, int y);
    bool swipe(const std::vector<Step>& steps);
    bool press_key(int key);

private:
    void scale_point(int& x, int& y);

    std::shared_ptr<InvokeApp> invoke_app_ = std::make_shared<InvokeApp>();
    std::shared_ptr<IOHandler> shell_handler_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    double xscale_ = 0;
    double yscale_ = 0;
    int press_ = 0;
};

MAA_CTRL_UNIT_NS_END
