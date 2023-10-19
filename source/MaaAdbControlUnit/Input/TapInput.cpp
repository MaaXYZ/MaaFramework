#include "TapInput.h"

#include "Utils/Logger.h"

#include <tuple>

MAA_CTRL_UNIT_NS_BEGIN

bool TapTouchInput::parse(const json::value& config)
{
    return parse_argv("Click", config, click_argv_) && parse_argv("Swipe", config, swipe_argv_);
}

bool TapTouchInput::init(int swidth, int sheight, int orientation)
{
    return set_wh(swidth, sheight, orientation);
}

bool TapTouchInput::set_wh(int swidth, int sheight, int orientation)
{
    std::ignore = swidth;
    std::ignore = sheight;
    std::ignore = orientation;

    return true;
}

bool TapTouchInput::click(int x, int y)
{
    LogInfo << VAR(x) << VAR(y);

    merge_replacement({ { "{X}", std::to_string(x) }, { "{Y}", std::to_string(y) } });

    LogDebug << VAR(x) << VAR(y);
    auto cmd_ret = command(click_argv_.gen(argv_replace_));

    return cmd_ret && cmd_ret->empty();
}

bool TapTouchInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    merge_replacement({ { "{X1}", std::to_string(x1) },
                        { "{Y1}", std::to_string(y1) },
                        { "{X2}", std::to_string(x2) },
                        { "{Y2}", std::to_string(y2) },
                        { "{DURATION}", duration ? std::to_string(duration) : std::string() } });
    auto cmd_ret = command(swipe_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

bool TapTouchInput::touch_down(int contact, int x, int y, int pressure)
{
    LogError << "TapTouchInput not supports" << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);
    return false;
}

bool TapTouchInput::touch_move(int contact, int x, int y, int pressure)
{
    LogError << "TapTouchInput not supports" << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);
    return false;
}

bool TapTouchInput::touch_up(int contact)
{
    LogError << "TapTouchInput not supports" << VAR(contact);
    return false;
}

bool TapKeyInput::parse(const json::value& config)
{
    return parse_argv("PressKey", config, press_key_argv_);
}

bool TapKeyInput::press_key(int key)
{
    LogInfo << VAR(key);

    merge_replacement({ { "{KEY}", std::to_string(key) } });
    auto cmd_ret = command(press_key_argv_.gen(argv_replace_));

    return cmd_ret.has_value() && cmd_ret.value().empty();
}

MAA_CTRL_UNIT_NS_END
