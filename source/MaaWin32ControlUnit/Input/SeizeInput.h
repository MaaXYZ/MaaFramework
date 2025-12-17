#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class SeizeInput : public InputBase
{
public:
    SeizeInput(HWND hwnd, bool block_input = false)
        : hwnd_(hwnd)
        , block_input_(block_input)
    {
    }

    virtual ~SeizeInput() override;

public: // from InputBase
    virtual MaaControllerFeature get_features() const override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool scroll(int dx, int dy) override;

private:
    void ensure_foreground();
    // 获取 last_pos_，若未设置则返回窗口客户区中心坐标
    std::pair<int, int> get_target_pos() const;

    HWND hwnd_ = nullptr;
    const bool block_input_ = false;

    std::pair<int, int> last_pos_;
    bool last_pos_set_ = false;
};

MAA_CTRL_UNIT_NS_END
