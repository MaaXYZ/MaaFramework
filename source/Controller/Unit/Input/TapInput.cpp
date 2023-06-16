#include "TapInput.h"

#include "Utils/Logger.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool TapInput::parse(const json::value& config)
{
    return parse_argv("Click", config, click_argv_) && parse_argv("Swipe", config, swipe_argv_) &&
           parse_argv("PressKey", config, press_key_argv_);
}

bool TapInput::click(int x, int y)
{
    LogFunc;

    merge_replacement({ { "{X}", std::to_string(x) }, { "{Y}", std::to_string(y) } });
    auto cmd_ret = command(click_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

bool TapInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogFunc;

    merge_replacement({ { "{X1}", std::to_string(x1) },
                        { "{Y1}", std::to_string(y1) },
                        { "{X2}", std::to_string(x2) },
                        { "{Y2}", std::to_string(y2) },
                        { "{DURATION}", std::to_string(duration) } });
    auto cmd_ret = command(swipe_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

bool TapInput::press_key(int key)
{
    LogFunc;

    merge_replacement({ { "{KEY}", std::to_string(key) } });
    auto cmd_ret = command(press_key_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

MAA_CTRL_UNIT_NS_END
