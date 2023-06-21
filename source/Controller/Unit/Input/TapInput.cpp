#include "TapInput.h"

#include "Utils/Logger.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool TapTouchInput::parse(const json::value& config)
{
    return parse_argv("Click", config, click_argv_) && parse_argv("Swipe", config, swipe_argv_);
}

bool TapTouchInput::click(int x, int y)
{
    LogFunc;

    merge_replacement({ { "{X}", std::to_string(x) }, { "{Y}", std::to_string(y) } });
    auto cmd_ret = command(click_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

bool TapTouchInput::swipe(const std::vector<Step>& steps)
{
    LogFunc;

    if (steps.size() < 2) {
        return false;
    }

    // TODO: 考虑调用多次
    merge_replacement({ { "{X1}", std::to_string(steps[0].x) },
                        { "{Y1}", std::to_string(steps[0].y) },
                        { "{X2}", std::to_string(steps[1].x) },
                        { "{Y2}", std::to_string(steps[1].y) },
                        { "{DURATION}", std::to_string(steps[0].delay) } });
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
