#pragma once

#include <optional>

#include "MaaFramework/MaaDef.h"
#include "MaaUtils/NoWarningCVMat.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapBase
{
public:
    virtual ~ScreencapBase() = default;

public:
    virtual std::optional<cv::Mat> screencap() = 0;

protected:
};

class InputBase
{
public:
    virtual ~InputBase() = default;

public:
    virtual MaaControllerFeature get_features() const = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) = 0;

    virtual bool touch_down(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_move(int contact, int x, int y, int pressure) = 0;
    virtual bool touch_up(int contact) = 0;

    virtual bool click_key(int key) = 0;
    virtual bool input_text(const std::string& text) = 0;

    virtual bool key_down(int key) = 0;
    virtual bool key_up(int key) = 0;

    virtual bool scroll(int dx, int dy) = 0;
};

MAA_CTRL_UNIT_NS_END
