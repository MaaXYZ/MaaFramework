#pragma once

#include <string>

#include "Conf/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class InputBase
{
public:
    virtual ~InputBase() = default;

    virtual bool init(int display_width, int display_height) = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;

    virtual bool click_key(int key) = 0;
    virtual bool input_text(const std::string& text) = 0;

    virtual bool key_down(int key) = 0;
    virtual bool key_up(int key) = 0;

    virtual bool start_app(const std::string& intent) = 0;
    virtual bool stop_app(const std::string& intent) = 0;
};

MAA_CTRL_UNIT_NS_END
