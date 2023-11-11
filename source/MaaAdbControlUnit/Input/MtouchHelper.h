#pragma once

#include "Base/UnitBase.h"

#include "Invoke/InvokeApp.h"

MAA_CTRL_UNIT_NS_BEGIN

class MtouchHelper : public TouchInputBase
{
public:
    virtual ~MtouchHelper() override = default;

public: // from TouchInputAPI
    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

protected:
    bool read_info(int swidth, int sheight, int orientation);
    virtual std::pair<int, int> screen_to_touch(int x, int y) = 0;
    virtual std::pair<int, int> screen_to_touch(double x, double y) = 0;

    std::shared_ptr<IOHandler> shell_handler_ = nullptr;

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
