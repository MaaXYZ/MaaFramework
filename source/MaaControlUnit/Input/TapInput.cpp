#include "TapInput.h"

#include "Utils/Logger.h"

#include <tuple>

MAA_CTRL_UNIT_NS_BEGIN

bool TapTouchInput::parse(const json::value& config)
{
    return parse_argv("Click", config, click_argv_) && parse_argv("Swipe", config, swipe_argv_);
}

bool TapTouchInput::init(int swidth, int sheight)
{
    std::ignore = swidth;
    std::ignore = sheight;
    return true;
}

void TapTouchInput::set_wh(int swidth, int sheight)
{
    init(swidth, sheight);
}

bool TapTouchInput::click(int x, int y)
{
    LogFunc;

    merge_replacement({ { "{X}", std::to_string(x) }, { "{Y}", std::to_string(y) } });

    LogDebug << VAR(x) << VAR(y);
    auto cmd_ret = command(click_argv_.gen(argv_replace_));

    return cmd_ret && cmd_ret->empty();
}

bool TapTouchInput::swipe(const std::vector<SwipeStep>& steps)
{
    LogFunc;

    if (steps.size() < 2) {
        LogError << VAR(steps.size());
        return false;
    }

    int delay_sum = 0;
    for (const auto& step : steps) {
        delay_sum += step.delay;
    }

    merge_replacement({ { "{X1}", std::to_string(steps.front().x) },
                        { "{Y1}", std::to_string(steps.front().y) },
                        { "{X2}", std::to_string(steps.back().x) },
                        { "{Y2}", std::to_string(steps.back().y) },
                        { "{DURATION}", std::to_string(delay_sum) } });
    auto cmd_ret = command(swipe_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

bool TapKeyInput::parse(const json::value& config)
{
    return parse_argv("PressKey", config, press_key_argv_);
}

bool TapKeyInput::press_key(int key)
{
    LogFunc;

    merge_replacement({ { "{KEY}", std::to_string(key) } });
    auto cmd_ret = command(press_key_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

MAA_CTRL_UNIT_NS_END
