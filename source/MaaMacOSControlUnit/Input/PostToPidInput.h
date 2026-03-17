#pragma once

#include "Base/UnitBase.h"
#include "Common/Conf.h"

#include <ApplicationServices/ApplicationServices.h>

MAA_CTRL_UNIT_NS_BEGIN

// 通过 CGEventPostToPid 将鼠标/键盘事件直接发送至目标进程，无需目标窗口处于前台。
class PostToPidInput : public InputBase
{
public:
    PostToPidInput(uint32_t window_id)
        : window_id_(window_id)
    {
        update_window_info();
    }

    virtual ~PostToPidInput() override = default;

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
    void update_window_info();

    // 工具函数：创建、发送并释放 CGEvent
    bool post_mouse_event(CGEventType type, int x, int y);
    bool post_keyboard_event(CGKeyCode key_code, bool key_down);

    uint32_t window_id_ = 0;
    pid_t pid_ = -1;
    int window_w_ = 0;
    int window_h_ = 0;
    int latest_touch_x_ = 0;
    int latest_touch_y_ = 0;
};

MAA_CTRL_UNIT_NS_END
